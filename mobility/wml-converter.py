import re
try:
    f = open("mobscenario.wml","r")
    x = f.read()
except IOError:
    f = open("mobscenario.wml","w")
f.close()

whole = x
whole = whole.replace("\n", "")
whole = whole.replace("\t", "")
time_data = re.split("<timestamp>\\d+.\\d+</timestamp>", whole)
time_data.remove('')
times = re.findall("<timestamp>(\\d+.\\d+)</timestamp>", whole)
node_numbers = []
node_positions = []
for e in time_data:
    current_numbers = re.findall('<node id="(\\d+)">', e)
##    for current_node in current_numbers:
    node_positions.append(re.findall('<x>(\\d+.\\d+)</x><y>(\\d+.\\d+)</y>',e))
    node_numbers.append(current_numbers)

result = "#node time(s) x y\n\n"
counter = 0
for time in times:
    current_line = ""
    c = 0
    for node in node_numbers[counter]:
        current_line += node + " " + time + " " + node_positions[counter][c][0] + " " + node_positions[counter][c][1] + "\n"
        c += 1

    counter += 1
    result += current_line
        
f = open("result.dat","w")
f.write(result)
f.close()

