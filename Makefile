out_name = catoptric_out

makecat: CatoptricSurface.cpp CatoptricRow.cpp prep_serial.cpp SerialFSM.cpp
	g++ -Wall --std=c++11 CatoptricController.cpp CatoptricSurface.cpp CatoptricRow.cpp prep_serial.cpp SerialFSM.cpp -o $(out_name)

clean:
	rm $(out_name) 2> /dev/null
