Simple tool to extract the resources stored in the custom DAT format used by the Into the Breach game.


DAT File format
---------------
1 x Header block

FBC x File blocks


Header block

Field			              Length

File block count (FBC)	4 bytes

File block offset list	4 bytes x FBC


File block

Field			            Length

File data len (FDL):	4 bytes

File name len (FNL):	4 bytes

File name (FN):		  	FNL

File data (FD):		  	FDL



License: You are free to use the code as you wish.
