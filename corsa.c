#include <stdio.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#define NUM_TAPPE 4
#define NUM_SQUADRE 10

struct corridore_t{
	int num_squadra;
	int num_tappa;
	int tempo_corsa;
};

struct tempi_t{	
	struct corridore_t corridore; 
	time_t istante_arrivo;							 				//Istante di arrivo alla tappa 
};

struct array_tempi_t {
	struct tempi_t array[NUM_SQUADRE*NUM_TAPPE];
	int head, tail;
	sem_t full, empty;
	sem_t mutex;
};

struct corsa_t{
	struct array_tempi_t tempi;
	time_t istante_partenza;										//Istante di inizio della gara
	sem_t preparazione_corridori;
	sem_t attesa_testimone[NUM_SQUADRE][NUM_TAPPE];				    //Attesa che il compagno di squadra mi passi il testimone
}corsa;

void init_array(struct array_tempi_t *a) {
	a->head=0; 
	a->tail=0;
	
	sem_init(&a->empty,0,0); 
	sem_init(&a->mutex,0,1);
}

void insert_array(struct array_tempi_t *a, struct tempi_t elem) {
	sem_wait(&a->mutex);
	a->array[a->head]=elem;
	a->head++;
	sem_post(&a->mutex);
	sem_post(&a->empty);
}

struct tempi_t extract_array(struct array_tempi_t *a) {
	struct tempi_t elem;
	sem_wait(&a->empty);
	sem_wait(&a->mutex);
	elem = a->array[a->tail];
	a->tail++;
	sem_post(&a->mutex);
	return elem;
}

void init_corsa(struct corsa_t* c){
	init_array(&c->tempi);
	
	sem_init(&c->preparazione_corridori,0,0);
	
	for (int i=0; i<NUM_SQUADRE; i++){
		for (int j=0; j<NUM_TAPPE; j++){
			sem_init(&c->attesa_testimone[i][j],0,0);
		}
	}
}

void attendi_corridori(struct corsa_t* c){
	for (int i=0; i<NUM_SQUADRE; i++)
		sem_wait(&c->preparazione_corridori);											//Aspetto che il primo corridore di ogni squadra sia pronto sulla linea del via	
}

void attendi_il_via(struct corsa_t* c, struct corridore_t corridore){
	if (corridore.num_tappa == 1)
		sem_post(&c->preparazione_corridori);
	sem_wait(&c->attesa_testimone[corridore.num_squadra-1][corridore.num_tappa-1]);		//Il primo di ogni squadra verrà sbloccato dall'arbitro, gli altri dal compagno che lo precede	
}

void via(struct corsa_t* c){
	for (int i=0; i<NUM_SQUADRE; i++)
		sem_post(&c->attesa_testimone[i][0]);						//Sblocco il primo corridore di ogni squadra
	c->istante_partenza = time(NULL);
}

void risultato_corsa(struct corsa_t* c){
	int squadre_arrivate = 0;
	for (int i=0; i<NUM_TAPPE*NUM_SQUADRE; i++){
		struct tempi_t tempo = extract_array(&c->tempi);			//Rimango in attesa del prossimo aggiornamento
		time_t tempo_trascorso = (tempo.istante_arrivo - c->istante_partenza);
		printf ("La squadra %d e' arrivata alla tappa %d in %lld secondi\n\n", tempo.corridore.num_squadra, tempo.corridore.num_tappa, tempo_trascorso);
		
		if (tempo.corridore.num_tappa == NUM_TAPPE){
			squadre_arrivate++;
			if (squadre_arrivate == 1)
				printf("La squadra %d ha vinto!\n\n", tempo.corridore.num_squadra);
			else if (squadre_arrivate == NUM_SQUADRE)
				printf("La squadra %d e' arrivata ultima!\n", tempo.corridore.num_squadra);
		}
	}
}

void sono_arrivato(struct corsa_t* c, struct corridore_t corridore){
	struct tempi_t tempo;
	time_t istante_arrivo = time(NULL);
	
	tempo.corridore = corridore;
	tempo.istante_arrivo = istante_arrivo;
	
	insert_array(&c->tempi, tempo);
	if (corridore.num_tappa != NUM_TAPPE)				//Se non sono l'ultimo, sblocco il compagno che mi segue
		sem_post(&c->attesa_testimone[corridore.num_squadra-1][corridore.num_tappa]);								
}

void *corridore(void *arg){
	struct corridore_t corridore = *((struct corridore_t *) arg);
	attendi_il_via(&corsa, corridore);
	
	sleep(corridore.tempo_corsa);								//Corri e raggiungi la prossima tappa
	
	sono_arrivato (&corsa, corridore);							
	
	return 0;
}

void *arbitro(void *arg){
	attendi_corridori(&corsa);
	sleep(1);												    //pronti, attenti
	
	via(&corsa);
	
	risultato_corsa(&corsa);									//Stampa i risultati parziali non appena disponibili
	return 0;
}

int main(){	
	struct corridore_t corridori[NUM_SQUADRE][NUM_TAPPE];
	pthread_attr_t a;
  	pthread_t thread_corridori[NUM_SQUADRE][NUM_TAPPE], thread_arbitro;
	
	//inizializzo i numeri casuali
	srand(100);
	
	pthread_attr_init(&a);
  	
	//inizializzo il mio sistema
	init_corsa(&corsa);
	
	for (int i=0; i<NUM_SQUADRE; i++){
		for (int j=0; j<NUM_TAPPE; j++){
			corridori[i][j].num_squadra = i+1;
			corridori[i][j].num_tappa = j+1;
			corridori[i][j].tempo_corsa = 1+rand()%5;
			//printf ("ID squadra: %d, ID tappa: %d\n", corridori[i][j].num_squadra, corridori[i][j].num_tappa);
		}
	}
	
	for (int i=0; i<NUM_SQUADRE; i++){
		for (int j=0; j<NUM_TAPPE; j++){
			pthread_create(&thread_corridori[i][j], &a, corridore, &corridori[i][j]);
		}
	}
	
	pthread_create(&thread_arbitro, &a, arbitro, NULL);
	
	for (int i=0; i<NUM_SQUADRE; i++){
		for (int j=0; j<NUM_TAPPE; j++){
			pthread_join(thread_corridori[i][j], NULL);
		}
	}
	
	pthread_join(thread_arbitro, NULL);
	
	printf("\n");

	pthread_attr_destroy(&a);

	//aspetto 1 secondo prima di terminare tutti quanti
    sleep(1);			
	
	return 0;
}
