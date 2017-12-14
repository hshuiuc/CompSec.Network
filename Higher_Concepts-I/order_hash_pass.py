import os

RECOV_DIR = 'hashes_passwords/'
ORIG_HASH = 'hashes.txt'
STORE_PWD = '5.2.4.txt'

hash_to_pass = {}

for recovered_file in os.listdir(RECOV_DIR):
	if recovered_file.startswith('.'):
		continue
	recf = open(RECOV_DIR + recovered_file)

	for line in recf:
		hash, password = line.split(':', 1)
		hash_to_pass[hash] = password

	recf.close()

# with open(RECOV_DIR+'recover_consolidated.txt', 'w') as cfile:
# 	for k, v in hash_to_pass.iteritems():
# 		cfile.write(k+':'+v)

print 'Unique hashes-to-password found:', len(hash_to_pass)

with open(ORIG_HASH, 'r') as hshf, open(STORE_PWD, 'w') as pwdf:
	for line in hshf:
		hash = line.rstrip('\n')

		if hash in hash_to_pass:
			pwdf.write(hash_to_pass[hash])
		else:
			pwdf.write("\n")
