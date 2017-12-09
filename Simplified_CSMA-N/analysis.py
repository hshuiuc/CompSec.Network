import matplotlib.pyplot as plt

start_N = 5
curve_color = ['']
	
def main():
	a_file = open('3a.txt', 'r')
	y_data = []
	for line in a_file:
		y_data.append(float(line))
	a_file.close()
	x_data = range(start_N, start_N + len(y_data))

	plt.plot(x_data, y_data)
	plt.autoscale(enable=True, axis='x', tight=True)
	plt.title('3a')
	plt.ylabel('Channel utilization (in percentage)')
	plt.xlabel('Number of nodes')

	plt.figure()
	b_file = open('3b.txt', 'r')
	y_data = []
	for line in b_file:
		y_data.append(float(line))
	b_file.close()

	plt.plot(x_data, y_data)
	plt.autoscale(enable=True, axis='x', tight=True)
	plt.title('3b')
	plt.ylabel('Channel idle fraction (in percentage)')
	plt.xlabel('Number of nodes')

	plt.figure()
	c_file = open('3c.txt', 'r')
	y_data = []
	for line in c_file:
		y_data.append(int(line))
	c_file.close()

	plt.plot(x_data, y_data)
	plt.autoscale(enable=True, axis='x', tight=True)
	plt.title('3c')
	plt.ylabel('Total number of collisions')
	plt.xlabel('Number of nodes')

	plt.figure()
	d_file = open('3d.txt', 'r')
	for count, line in enumerate(d_file):
		y_data = [float(y) for y in line.split(',')]
		plt.plot(x_data, y_data, label='initial R = '+str(2**count))

	plt.autoscale(enable=True, axis='x', tight=True)
	plt.legend(loc='best')
	plt.title('3d')
	plt.ylabel('Channel utilization (in percentage)')
	plt.xlabel('Number of nodes')

	plt.figure()
	e_file = open('3e.txt', 'r')
	for count, line in enumerate(e_file):
		y_data = [float(y) for y in line.split(',')]
		plt.plot(x_data, y_data, label='packet L = '+str(20*(count+1)))

	plt.autoscale(enable=True, axis='x', tight=True)
	plt.legend(loc='best')
	plt.title('3e')
	plt.ylabel('Channel utilization (in percentage)')
	plt.xlabel('Number of nodes')

	plt.show()

if __name__ == "__main__":
	main()
