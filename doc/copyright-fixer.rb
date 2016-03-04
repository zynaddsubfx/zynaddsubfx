require "open3"

base = ARGV[0]
Dir.chdir base
all_files = Dir.glob('**/*').select {|f| File.file?(f)}
src_files = all_files.select {|f| f.match(/\.(cpp|H|h|C)$/)}
zyn_files = src_files.select {|f| f.match(/^src/)}

def add_license(fname)
    puts "Please Enter a description for #{fname}:"
    short = fname.match(/.*\/(.*)/)[1]

    desc = $stdin.gets.strip
    str  = %{/*
  ZynAddSubFX - a software synthesizer

  #{short} - #{desc}
  Copyright (C) 2016 Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/}

    Open3.popen3("ed -p: #{fname}") do |stdin, stdout, stderr|
        stdin.puts "H"
        stdin.puts "0a"
        stdin.puts str
        stdin.puts "."
        stdin.puts "w"
        stdin.puts "q"
        puts stdout.gets
    end
end

def update_license(fname, first, last)
    str = %{  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/}
    Open3.popen3("ed -p: #{fname}") do |stdin, stdout, stderr|
        stdin.puts "H"
        stdin.puts "#{first},#{last}c"
        stdin.puts str
        stdin.puts "."
        stdin.puts "w"
        stdin.puts "q"
        puts stdout.gets
    end
end

tmp = 5000
zyn_files.sort.each do |fname|
    if tmp > 0
        tmp -= 1
    else
        exit
    end

    if(fname.match(/jack_osc/) || fname.match(/NSM/))
        next
    end

    f = File.open(fname, "r")
    line = 0
    puts
    puts "Info: Processing '#{fname}'"

    #Check for Copyright Header
    first = f.gets
    line += 1

    if(!first.match /\/\*/)
        puts "Error: No Copyright Header Found"
        f.close
        add_license(fname)
        next
    end

    #Program Title
    ln = f.gets
    line += 1

    if(!ln.match /ZynAddSubFX - a software synthesizer/)
        puts "Warning: Unexpected Program Name"
    end

    #Empty
    ln = f.gets
    line += 1

    if(!ln.match /^$/)
        puts "Warning: Broken Formatting"
    end

    #File Description
    ln = f.gets
    line += 1
    if(!ln.match /(.*) - (.*)/)
        puts "Warning: Invalid Description"
    end

    #Copyright Section
    copying = true
    while copying
        ln = f.gets
    line += 1
        if(ln.match(/^          .*/))
            next
        end
        if(ln.match(/^$/)|| ln.match(/Author/))
            break
        end
        if(!(ln.match(/Copyright \(C\) ([0-9]*)-([0-9]*) (.*)/) ||
             ln.match(/Copyright \(C\) ([0-9]*) (.*)/)))
            puts "Warning: Invalid Copyright Field"
            puts "         <#{ln}>"
        end
    end
    

    #Out-Of-Date Author Section
    if(ln.match /Author/)
        while true
            ln = f.gets
            line += 1
            if(ln.match(/^$/))
                break
            end
        end
    end

    #Completely Standard Copyright Stuff
    ln = f.gets
    line += 1
    if(ln.match /This program is free software/)
        puts "Info: GPL Found..."
        initial_line = line
        while(!ln.match(/\*\//))
            ln = f.gets
            line += 1
            if(ln.downcase.match(/version 2/))
                puts ln
            end
        end
        final_line = line
        f.close
        update_license(fname, initial_line, final_line)
        puts "Info: GPL Lines #{initial_line} #{final_line} total #{final_line-initial_line}"
        next
    else
        puts "Error: Invalid Copyright Header"
        puts "       Line = <#{ln}>"
        puts `head #{fname}`
    end
end
#zyn_files = src_files.select
#puts Dir.glob('**/*').select { |f| !!(File.file?(f) && f.match(/\.(cpp|H|h|C)$/))}
