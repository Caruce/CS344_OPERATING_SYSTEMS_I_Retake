#compile the program by using the following command
gcc --std=gnu99 -o line_processor line_processor.c -lpthread

#run the program by using the following command
./line_processor < input1.txt > output1.txt
./line_processor < input2.txt > output2.txt
./line_processor < input3.txt > output3.txt
