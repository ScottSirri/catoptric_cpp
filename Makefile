makecat: CatoptricSurface.cpp CatoptricRow.cpp prep_Serial.cpp SerialFSM.cpp
	g++ --std=c++11 CatoptricSurface.cpp CatoptricRow.cpp prep_serial.cpp SerialFSM.cpp -o catout

clean:
	rm catout 2> /dev/null