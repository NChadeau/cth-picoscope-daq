##################################################################################
###                       Pre-analysis and Analysis                            ###
###                               01/08/2024                                   ###
###                          (still in progress)	                           ###
##################################################################################

### source files:
	- /src/analysis/main.cpp (main analysis file)
### include files:
	- /include/analysis/constants.h (constants and parameters)
### executables:
	- /exec/analysis (executable generated after compiling)
	- /exec/launch_analysis.sh (bash script executable used to launch pre-analysis and analysis)
	
### How to do:
	- Launch a pre-analysis:
		./analysis pre-analyse yyyy-mm-dd MPPC1-MPPC2-MPPC3 /path/to/file path/to/where_to_be_saved
	- Launch analysises:
		* To have MEAN MPPC response and MEAN PMT response versus LED voltage
			./analysis analyse maxMPPC-maxPMT yyyy-mm-dd MPPC1-MPPC2-MPPC3 /path/to/file path/to/where_to_be_saved
		* To test reproducibility (in progress...)
			./analysis analyse reproducibility yyyy-mm-dd MPPC1-MPPC2-MPPC3 /path/to/file path/to/where_to_be_saved
		* To test position dependency (in progress...)
			./analysis analyse position_dependency yyyy-mm-dd MPPC1-MPPC2-MPPC3 /path/to/file path/to/where_to_be_saved
		* Final Quality Control (in progress...)
			./analysis analyse QC yyyy-mm-dd MPPC1-MPPC2-MPPC3 /path/to/file path/to/where_to_be_saved

