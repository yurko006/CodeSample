CSci 4061 Spring 2015 Assignment 3
Name1=Nikita Yurkov
Name2Colin Heim
StudentID1: yurko006  4411081
Student ID Colin: heimx079  4629776


Usage Instructions:

      make clean
      make all
      ./test input_directory output_dir filesystem.log


Anomalies Not Handled:

-Our program does not handle traversing through a nested input directory. It will only take the ones that are in the main input directory.
-Our file makes thumbnails within the html and those render correctly, but we were unable to place them in the output directory.       
-Because of a line in mini_filesystem.c, the input file needs to be called input_directory. If it is called input_dir or something along those lines, it will not move and render the files properly.

Experiment Results: 

Our program’s lower level functions were accessed a grand total of 10875 times. If we calculate further, this means there are a total of 777 accesses per file. The following will be assumed in order to calculate the best and average case.


We can infer that there are 5437 blocks which hold all the files. 


Taverage rotation = .5 * (60/15000) * 1000 = 2ms
Tmax rotation = 4 ms
Taverage seek = 4 ms


Best Case:

Taverage seek + Taverage rotation + Tmax rotation = 4 + 2 + 4 = 10ms


Average Case:

(Taverage seek + Taverage rotation) * 5437 = 8 * 5437 = 43.496 seconds


It is clear as to why we need to defragment our hard drives.
