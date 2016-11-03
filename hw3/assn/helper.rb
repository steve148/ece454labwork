arr = File.read("temp.txt").split("\n").map {|s| s[7..-1].to_i }

arr.uniq.each do |n|
	puts "#{n} #{arr.count(n)}"
end
