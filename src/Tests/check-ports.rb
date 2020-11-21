#!/usr/bin/ruby

require 'open3'

rval=0
my_path=File.dirname(__FILE__)

# start zyn, grep the lo server port, and connect the port checker to it
# NOTE: If you add too much debug output to zyn, it will be blocked when writing into the pipe,
#       leading to MiddleWare to not reply and to port-checker to fail.
#       This script should be rewritten to keep reading (and printing) zyn's messages.
Open3.popen3(my_path + "/../zynaddsubfx -O null --no-gui") do |stdin, stdout, stderr, wait_thr|
  pid = wait_thr[:pid]
  while line=stderr.gets do 
    # print "line: " + line;
    if /^lo server running on (\d+)$/.match(line) then
      sleep 3 # give zyn more time to setup
      port_checker_rval = system(my_path + "/../../rtosc/port-checker 'osc.udp://localhost:" + $1 + "/'")
      if port_checker_rval != true then
        puts "Error: port-checker has returned #{$?.exitstatus}."
        rval=1
      end
      begin
        # Check if zyn has not crashed yet
        # Wait 1 second before detection
        sleep 1
        Process.kill(0, pid)
        # This is only executed if Process.kill did not raise an exception
        Process.kill("KILL", pid)
      rescue Errno::ESRCH
        puts "Error: ZynAddSubFX (PID #{pid}) crashed!"
        puts "       Missing replies or port-checker crashes could be due to the crash."
        rval=1
      rescue
        puts "Error: Cannot kill ZynAddSubFX (PID #{pid})?"
        rval=1
      end
    end
  end
end

exit rval
