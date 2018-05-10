#!/usr/bin/python
import csv
import json

json_filename = "envs_small.txt"

columns = []

with open('betree_defs', 'rb') as csvfile:
    reader = csv.reader(csvfile, delimiter='|')
    for row in reader:
        columns.append(row[0])

with open('betree_events', 'w') as csvfile:
    with open(json_filename, 'rb') as jsonfile:
        lines = jsonfile.readlines()
        for line in lines:
            data = json.loads(line)
            i = 0
            for column in columns:
                if column in data:
                    value = data[column]
                else:
                    ''
                if i != 0:
                    csvfile.write("|")
                csvfile.write(str(value))
                i+=1
            csvfile.write("\n")
