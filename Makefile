makecat: CatoptricRow.cpp prep_Serial.cpp SerialFSM.cpp
	g++ CatoptricRow.cpp prep_serial.cpp SerialFSM.cpp -o catout

clean:
	rm catout 2> /dev/null
