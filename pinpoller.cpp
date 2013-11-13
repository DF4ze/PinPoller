/******************************************************************************************************************\
		
		-----=====	PinPoller =====-----
	
	Va poller un pin GPIO d'un Raspberry Pi toute les 500 milliseconds par défaut
	Va alerter en fonction des paramètres :
	 - (Edge) sur un front montant, descendant ou les 2.
	 - en continu si le mode boucle est activé
	 - en affichant plus ou moins d'information en fonction de la verbosité

	Une place est laissée libre pour ajouter des actions personnelles dans la fonction "void action_perso( void )" 
	... Pratique en mode boucle et sans verbosité.
	Mais peut etre utilisé en script en retirant le mode boucle et affichant une verbosité minimum (1)
	
	
	Pour utiliser ce source, il est necessaire soit d'installer le paquet libboost-dev, 
	soit de récupérer les sources asio.hpp et posix_time.hpp
	
	bug(s) : 
		- en mode boucle, ne ferme pas proprement le GPIO car on est obligé de killer le programme.
		- ne prend pas le X de l'option -v ....
	
	sources :
		- http://www.blaess.fr/christophe/2012/11/26/les-gpio-du-raspberry-pi/
		- http://www.boost.org/
		  plus préscisément : http://www.boost.org/doc/libs/1_35_0/doc/html/boost_asio/tutorial/tuttimer1.html
		
	
	Développé par Clément ORTIZ (08/11/2013), Open Source ;)
	

	

\*******************************************************************************************************************/




#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>


int bDebug = 0;						// affiche-t-on les infos de debug.
char sFileName[255]; 				// nom du fichier à checker.


/////////////////////////////////////
// Gestion des PIN
void open_pin( char* sPin ){
	FILE *f;
	
	// Déclaration du PIN
	char sCommand[255];
	strcpy( sCommand, "echo " );
	strcat( sCommand, sPin );
	strcat( sCommand, " > /sys/class/gpio/export" );
	if( bDebug ){
		std::cout << "Déclaration du Pin : ";
		std::cout << sCommand;
		std::cout << "\n";
	}
	f = popen( sCommand, "r");
	fclose(f);
	
	// Sens du PIN
	strcpy( sCommand, "echo in > /sys/class/gpio/gpio" );
	strcat( sCommand, sPin );
	strcat( sCommand, "/direction" );
	if( bDebug ){
		std::cout << "Sens du Pin : ";
		std::cout << sCommand;
		std::cout << "\n";
	}
	f = popen( sCommand, "r");
	fclose(f);
	
	// Fichier a surveiller
	strcpy( sFileName, "/sys/class/gpio/gpio" );
	strcat( sFileName, sPin );
	strcat( sFileName, "/value" );
	if( bDebug ){
		std::cout << "Fichier Pin : ";
		std::cout << sFileName;
		std::cout << "\n";
	}
}
void reset_pin( char* sPin ){
	// Sens du PIN out
	char sCommand[255];
	strcpy( sCommand, "echo out > /sys/class/gpio/gpio" );
	strcat( sCommand, sPin );
	strcat( sCommand, "/direction" );
	
	FILE *f = popen( sCommand, "r");
	fclose(f);	
	
	// Sens du PIN in
	strcpy( sCommand, "echo in > /sys/class/gpio/gpio" );
	strcat( sCommand, sPin );
	strcat( sCommand, "/direction" );
	
	f = popen( sCommand, "r");
	fclose(f);	
	
	if( bDebug ){
		std::cout << "Reset du Pin : ";
		std::cout << sPin;
		std::cout << "\n";
	}

}
void close_pin( char* sPin ){
	FILE *f;
	
	// Déclaration du PIN
	char sCommand[255];
	strcpy( sCommand, "echo " );
	strcat( sCommand, sPin );
	strcat( sCommand, " > /sys/class/gpio/unexport" );
	if( bDebug ){
		std::cout << "Fermeture du Pin : ";
		std::cout << sCommand;
		std::cout << "\n";
	}
	f = popen( sCommand, "r");
	fclose(f);
}

////////////////////////////////////
// Déclaration Fonction(s) Perso
void action_perso( void );

int main(int argc,char **argv)
{

	////////////////////////////////
	// Parsing des parametres.
	long lTime = 		500; 			// temps d'attente en sec.
	int bVerbose = 		0;				// affiche-t-on le texte lu?
	bool bBoucle = 		0;				// boucle-t-on lorsqu'on detecte une différence?
	bool bQuit = 		0;				// Quitte-t-on la boucle?
	int iEdge = 		3;				// Declenchement de l'écoute sur le front montant(X=1), descendant(X=0) ou les 2(X=3)
	char* sPin;							// si bPin == 1 ==> String contenant le N° du PIN
	char* buffer_old;					// Ancienne valeur du buffer pour la comparaison QUAND il n'y a pas d'écrasement

	// Pour checker si -p a bien été demandé
	strcpy( sFileName, "file.test" );
	
	
	for( int i=1; i < argc; i++ ){
		switch(argv[i][1]){
			case 'h':
				std::cout << "t_synchro -[option] [valeur] ... \n\n";
				std::cout << " -- PARAM OBLIGATOIRE --\n";
				std::cout << "-p X :   PARAM OBLIGATOIRE, pour superviser le pin N°X (N° du GPIO et non N° WiringPi ;))\n\n";
				std::cout << " -- PARAMS OPTIONNELS --\n";
				std::cout << "-t X :   cadence le timer à X milliseconds ( ";
				std::cout << lTime;
				std::cout << " par defaut )\n";
				std::cout << "-b :     boucle lorsque détecte un changement ( 0 par defaut )\n";
				std::cout << "-e X :   Edge, déclenche sur le front montant(X=1), descendant(X=0) ou les 2(X=3)  ( 3 par defaut )\n\n";
				std::cout << " -- PARAMS AFFICHAGES --\n";
				std::cout << "-v [X] : Affiche le texte lu dans le fichier, [X] est optionnel\n         X = 0, 1 ou 2 -> + ou - de Verbosity ( 0 par defaut, 1 quand l'option est appelée)\n";
				std::cout << "-d :     Affiche les infos de debug ( 0 par defaut)\n";
				std::cout << "-h :     commande d'aide \n\n";
				exit(0);
			break;
			case 't':
				lTime = atol( argv[++i] ); // on récupère le temps en seconde et on incrémente le compteur d'argument
			break;
			case 'v':
				bVerbose = 1;
				// il n'est pas obligatoire de mettre un chiffre apres l'option -v, mais s'il y a en un alors bVerbose doit le retenir.
				if( i+1 <= argc ){
					if( argv[i+1][0] >= '0' &&  argv[i+1][0] >= '9' ){
						i++;
						bVerbose = argv[i][0] - '0';
					}
				}
			break;
			case 'd':
				bDebug = 1;
			break;
			case 'b':
				bBoucle = 1;
			break;
			case 'e':
				iEdge = atoi( argv[++i] );
			break;
			case 'p':
				sPin = argv[++i];
				open_pin( sPin );
			break;
			default:
				std::cout << "Parametre(s) incorrect(s), ";
				std::cout << argv[0];
				std::cout << " -h pour avoir de l'aide\n";
				exit(4);
			break;
		}
	} 
	
	// check si on a bien initialisé le PIN
	if( strcmp( sFileName, "file.test" ) == 0 ){
		std::cout << "Parametre(s) incorrect(s), ";
		std::cout << argv[0];
		std::cout << " -h pour avoir de l'aide\n";
		std::cout << "-p X est obligatoire\n\n";
		exit(4);
	}
	
	// Affichage de debug
	if( bDebug ){
		std::cout << "Parametres :\n";
		std::cout << "- Pin : ";
		std::cout << sPin;
		std::cout << "\n";
		std::cout << "- Fichier : ";
		std::cout << sFileName;
		std::cout << "\n";
		std::cout << "- Timer : ";
		std::cout << lTime;
		std::cout << " milliseconds\n";
		std::cout << "- Boucle : ";
		std::cout << bBoucle;
		std::cout << " \n";
	}
	// Info Verbosity
	if( bVerbose > 1 || bDebug){
		std::cout << "(Attente de modification du fichier)\n";
	}
	
	
	// On initialise le buffer_old avec la valeur actuelle du fichier.
	FILE * pFile;
	long lSize;

	// On ouvre le fichier
	pFile = fopen ( sFileName , "r" );
	if (pFile==NULL) {fputs ("File error\n",stderr); exit (1);}

	// Récupère la taille du fichier
	fseek (pFile , 0 , SEEK_END);
	lSize = ftell (pFile);
	rewind (pFile);
	
	// Allocation de la memoire en variable pour la taille du fichier
	buffer_old = (char*) malloc (sizeof(char)*(lSize+1));
	if (buffer_old == NULL) {fputs ("Memory error\n",stderr); exit (2);}

	// Ajout du 0 de fin de chaine
	buffer_old[ lSize ] = '\0';
	
	// Lecture du fichier
	fread (buffer_old,1,lSize,pFile);
	// Fermeture du fichier en lecture
	fclose (pFile);

	
	
	
	//////////////////////////
	// Timer
	do{
		// Init du timer a 5sec
		boost::asio::io_service io;
		boost::asio::deadline_timer t(io, boost::posix_time::milliseconds(lTime));
		// attente
		t.wait();
		
		
		//////////////////////////////
		// Fichier
		// FILE * pFile;
		// long lSize;
		char * buffer;
		size_t result;

		// On ouvre le fichier
		pFile = fopen ( sFileName , "r" );
		if (pFile==NULL) {fputs ("File error\n",stderr); exit (1);}

		// Récupère la taille du fichier
		fseek (pFile , 0 , SEEK_END);
		lSize = ftell (pFile);
		rewind (pFile);
		if( bDebug ){
			std::cout << "Taille du fichier : ";
			std::cout << lSize;
			std::cout << "\n";
		}
		
		// si le fichier n'est pas vide.
		if( lSize != 0 ){
			// Allocation de la memoire en variable pour la taille du fichier
			buffer = (char*) malloc (sizeof(char)*(lSize+1));
			if (buffer == NULL) {fputs ("Memory error\n",stderr); exit (2);}

			// Ajout du 0 de fin de chaine
			buffer[ lSize ] = '\0';
			
			// Lecture du fichier
			result = fread (buffer,1,lSize,pFile);
			// Fermeture du fichier en lecture
			fclose (pFile);

			// reset le pin apres l'avoir lu. Sinon la valeur reste bloquée
			reset_pin( sPin );
			
			// On compare avec l'ancienne chaine
			if(strcmp( buffer, buffer_old ) != 0){
				bool bAlert = false;
				/// Si changement, on mémorise cette nouvelle chaine
				// On purge le buffer_old
				//free (buffer_old);
				// et on copy
				strcpy( buffer_old, buffer );

				// Si le Edge correspond
				if( iEdge != 3 ){
					if( iEdge == atoi( buffer ) )
						bAlert = true;
				}else
					bAlert = true;
				
				if( bAlert ){
					// Si on ne boucle pas... alors on sort.				
					if( !bBoucle )
						bQuit = 1;
					
					// Affichage du contenu du fichier
					if( bVerbose || bDebug ){
						if( bVerbose > 1 || bDebug )
							std::cout << "Buffer : ";
						if( bVerbose > 0 || bDebug )
							std::cout << buffer;
						// if( bVerbose > 0 || bDebug )
							// std::cout << "\n";
						if( bVerbose > 1 || bDebug )
							std::cout << "\n(Attente de modification du fichier)\n";
						std::cout.flush();
					}
					
					action_perso();
				}				
			}
			
			
			// On purge le buffer
			free (buffer);
		}else
			fclose (pFile);
	
	}while( !bQuit );
	
	// Fermeture du Pin
	close_pin( sPin );
	
	return 0;
}




////////////////////////////////////
// Déclaration Fonction(s) Perso
void action_perso( void ){
	// Un peu de code a faire lors de la detection
	;
}
