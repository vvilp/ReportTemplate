
file = open("references.bib", "r")
output = open("references-without-url.bib", "w")

for line in file:
    if "url" not in line:
        output.write(line)

    
