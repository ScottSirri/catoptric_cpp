
#include "CatoptricRow.hpp"
#include "CatoptricSurface.hpp"

struct SerialPort {
    string 

serialPortOrder = { "8543931323035121E170" : 1,
					"8543931323035130C092" : 2,
					"85439313230351610262" : 3,
					"75435353934351D052C0" : 4,
					"85436323631351509171" : 5,
					"75435353934351F0C020" : 6,
					"8543931333035160E081" : 7,
					"85439313230351818090" : 8,
					"755333434363519171F0" : 9,
					"8543931333035160F102" : 10,
					"8543931323035161B021" : 11,
					"85439313330351D03160" : 12,
					"85439303133351716221" : 13,
					"85436323631351300201" : 14,
					"75435353934351E07072" : 15,
					"8543931323035170D0C2" : 16,
					"854393133303518072A1" : 33,
				}


class CatoptricSurface():

	def __init__(self):
		self.serialPorts = self.getOrderedSerialPorts()
		self.numRows = len(self.serialPorts)
		self.rowInterfaces = dict()

		self.setupRowInterfaces()
		self.reset() 


	# Initializes a Row Interface for each available arduino
	def setupRowInterfaces(self):

		for sP in self.serialPorts:
			name = serialPortOrder[sP.serial_number]
			port = sP.device

			rowLength = 0
			if (name >= 1 and name < 12):
				rowLength = 16						### THIS SHOULD BE 16, NOT 10 ###
			elif (name >= 12 and name < 17):
				rowLength = 24
			elif (name >= 17 and name < 28):
				rowLength = 17
			elif (name >= 28 and name < 33):
				rowLength = 25
