Programming Assignment E
CIS 525
Christian Csencsits

Description:
	This program was built inorder to familiarize ourselves with the HTTP
	protocols. My outputs and results were fairly standard. If i tried to
	connect to a site in which it had moved, it displayed an error "301
	moved permenantely". If the site gave bad request it displayed "400 
	bad request". If it connects fully, then it displays the propper html
	info. 
	
Compile & Run instructions:
		gcc -o client httpClient.c
		./client WEBPAGE PORT(optional)