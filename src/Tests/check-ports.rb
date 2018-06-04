#!/usr/bin/ruby

require 'open3'

# start zyn, grep the lo server port, and connect the port checker to it
Open3.popen3(Dir.pwd + "/../zynaddsubfx -O null --no-gui") do |stdin, stdout, stderr, wait_thr|
  pid = wait_thr[:pid]
  while line=stderr.gets do 
    # print "line: " + line;
    if /^lo server running on (\d+)$/.match(line) then
      rval = system(Dir.pwd + "/../../rtosc/port-checker 'osc.udp://localhost:" + $1 + "/'")
      Process.kill("KILL", pid)
      exit rval
    end
  end
end
