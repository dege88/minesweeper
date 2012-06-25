#ifdef __cplusplus
    #include <cstdlib>
#else
    #include <stdlib.h>
#endif
#ifdef __APPLE__
#include <SDL/SDL.h>
#else
#include <SDL.h>
#endif

#include <string>
#include <time.h>
#include <SDL_ttf.h>

#define images_number 14

class ccampo {
    private:
    int **mine; //array che contiene le mine
    int **local_campo; //array che contiene le informazioni grafiche/di gioco
    int height,width; //altezza e larghezza in caselle
    int numero_mine;
    int caselle_libere;
    int gameover; //gameover
    //mousedown variables
    int mousedown; //mouse cliccato
    int mousedown_x; //coordinate di click in caselle
    int mousedown_y;
    int mousedown_old_value; //vecchio valore (local_campo) della casella cliccata
    Uint8 mousedown_button; //bottone del mouse
    //graphic variables
    SDL_Surface* images_bmps[images_number]; //array che contiene le immagini caricate
    SDL_Surface* screen; //form

    //private functions
    int mine_count(int x, int y); //conta le mine
    int isMine(int x, int y); //check mina
    void markMines(int x, int y); //segna tutte le mine dopo un gameover
    void freeCell(int x, int y); //libera una cella che non contiene una mina
                                 //se ha 0 mine adiacenti avvia la pulizia ricorsiva
    SDL_Surface* load_image(char *filename); //carica e ottimizza un immagine

    public:
    ccampo(int height,int width,int numero_mine); //inizializzazione
    ~ccampo(); //deallocazione
    void draw_campo(SDL_Surface* screen, int x, int y); //funzione di disegno del campo di gioco
    void mousedown_event(int x,int y,Uint8 button); //eventi per i click del mouse
    void mouseup_event(int x,int y,Uint8 button);
    int get_width(); //larghezza
    int get_height(); //altezza
    int isGameover(); //il gioco è in gmaeover?
};

ccampo::ccampo(int height,int width,int numero_mine)
{
    //inizializzazione del seme casuale
    srand(time(NULL));

    //inizializzazione delle variabili interne
    (*this).height = height;
    (*this).width = width;
    (*this).numero_mine = numero_mine;

    //inizializzazione degli array per il campo
    (*this).local_campo = (int**) malloc(sizeof(int*) * width);
    (*this).mine = (int**) malloc(sizeof(int*) * width);
    if (((*this).local_campo == NULL) || ((*this).mine == NULL)) //controllo del risultato del malloc
    {
        printf("errore di allocazione!");
        exit(1);
    }

    for (int i=0;i<width;i++)
    {
        (*this).local_campo[i] = (int*) malloc(sizeof(int) * height);
        (*this).mine[i] = (int*) malloc(sizeof(int) * height);
        if (((*this).local_campo[i] == NULL) || ((*this).mine[i] == NULL)) //controllo del risultato del malloc
        {
            printf("errore di allocazione!");
            exit(1);
        }
        for(int j=0;j<height;j++) //inizializzazione della griglia e azzeramento delle mine
        {
            (*this).local_campo[i][j] = 9;
            (*this).mine[i][j] = 0;
        }
    }

    //inizializzazione delle mine
    if(numero_mine >= (width*height))
    {
        printf("errore: numero di mine fuori dai limiti!");
        exit(2);
    }
    while(numero_mine > 0)
    {
        int x,y;
        x = rand() % width;
        y = rand() % height;
        if((*this).mine[x][y] != 1)
        {
            (*this).mine[x][y] = 1;
            numero_mine--;
        }
    }

    //inizializzazione delle immagini
    //SDL_Surface* images_bmps[images_number];
    char* images_names[images_number] = {"0","1","2","3","4","5","6","7","8","void","flag","qmark","mine","exploded"};
    //                                    0   1   2   3   4   5   6   7   8    9      10     11      12       13
    char *tmp_images_name_ptr = (char*)malloc(200);
    for(int i=0;i<images_number;i++)
    {
        sprintf(tmp_images_name_ptr,"tiles/%s.bmp",images_names[i]);
        (*this).images_bmps[i] = SDL_LoadBMP(tmp_images_name_ptr);
        if (!(*this).images_bmps[i])
        {
            printf("Unable to load bitmap %s.bmp: %s\n", images_names[i], SDL_GetError());
            exit(3);
        }
    }
    free(tmp_images_name_ptr);

    //inizializzazione del video delle SDL
    if ( SDL_Init( SDL_INIT_VIDEO ) < 0 )
    {
        printf( "Unable to init SDL: %s\n", SDL_GetError() );
        return 4;
    }

    // make sure SDL cleans up before exit
    atexit(SDL_Quit);

    // create a new window
    SDL_Surface* screen = SDL_SetVideoMode(width * 16, height * 16, 16, SDL_HWSURFACE|SDL_DOUBLEBUF);wwwwwwww

    if ( !screen )
    {
        printf("Errore di inizializzazione del form: %s\n", SDL_GetError());
        return 1;
    }

    (*this).mousedown = 0;
    (*this).gameover = 0;
}

ccampo::~ccampo()
{
    //free allocated arrays
    for(int i=0;i<width;i++)
    {
        free((*this).mine[i]);
        free((*this).local_campo[i]);
    }
    free((*this).mine);
    free((*this).local_campo);

    // free loaded bitmaps
    for(int i=0;i<images_number;i++)
    {
        SDL_FreeSurface((*this).images_bmps[i]);
    }
}

void ccampo::draw_campo(SDL_Surface* screen, int x, int y)
{
    SDL_Rect dstrect;
    // clear screen
    SDL_FillRect(screen, 0, SDL_MapRGB(screen->format, 200, 190, 165));

    // draw bitmaps
    dstrect.x = x;
    dstrect.y = y;
    for(int i = 0;i<height;i++)
    {
        for(int j = 0;j<width;j++)
        {
            SDL_BlitSurface(images_bmps[(*this).local_campo[j][i]], 0, screen, &dstrect);
            dstrect.x = dstrect.x + 16;
        }
        dstrect.y = dstrect.y + 16;
        dstrect.x = 0;
    }

    // finally, update the screen :)
    SDL_Flip(screen);
}

void ccampo::mousedown_event(int x,int y,Uint8 button)
{
    if (((*this).local_campo[x][y] == 9) || ((*this).local_campo[x][y] == 10) || ((*this).local_campo[x][y] == 11))
    {
        (*this).mousedown = 1;
        (*this).mousedown_x = x;
        (*this).mousedown_y = y;
        (*this).mousedown_old_value = (*this).local_campo[x][y];
        (*this).mousedown_button = button;
        (*this).local_campo[x][y] = 0;
    }
}

void ccampo::mouseup_event(int x, int y, Uint8 button)
{
    if((*this).mousedown)
    {
        if(((*this).mousedown_x == x) && ((*this).mousedown_y == y))
        {
            if((*this).gameover)
            {
                (*this).local_campo[x][y] = (*this).mousedown_old_value;
            }
            else if((button == SDL_BUTTON_LEFT) && (((*this).mousedown_old_value == 9) || ((*this).mousedown_old_value == 11)))
            {
                if((*this).isMine(x,y))
                {
                    (*this).gameover = 1;
                    (*this).markMines(x, y);
                }
                else
                    (*this).freeCell(x,y);
            }
            else if((button == SDL_BUTTON_LEFT) && ((*this).mousedown_old_value == 10))
            {
                (*this).local_campo[x][y] = 10;
            }
            else if((button == SDL_BUTTON_RIGHT) && (((*this).mousedown_old_value == 9) || ((*this).mousedown_old_value == 10) || ((*this).mousedown_old_value == 11)))
            {
                (*this).local_campo[x][y] = (((*this).mousedown_old_value - 8) % 3) + 9; //cicla le tre modalita' (vuoto,bandiera,punto interrogativo)
            }
        }
        else
        {
            (*this).local_campo[mousedown_x][mousedown_y] = (*this).mousedown_old_value;
        }
        (*this).mousedown = 0;
    }
}

int ccampo::get_width()
{
    return (*this).width;
}

int ccampo::get_height()
{
    return (*this).height;
}

int ccampo::mine_count(int x, int y)
{
    int count = 0;

    int xmin,xmax,ymin,ymax;
    if (x == 0) xmin = 0;
    else xmin = x - 1;
    if (x == ((*this).width - 1)) xmax = x;
    else xmax = x + 1;

    if (y == 0) ymin = 0;
    else ymin = y - 1;
    if (y == ((*this).height - 1)) ymax = y;
    else ymax = y + 1;

    for(int i = xmin; i <= xmax; i++)
        for(int j = ymin; j <= ymax; j++)
            count += (*this).mine[i][j];
    return count;
}

int ccampo::isMine(int x, int y)
{
    return (*this).mine[x][y];
}

void ccampo::markMines(int x, int y)
{
    for(int i = 0; i < (*this).width; i++)
        for(int j = 0; j < (*this).height; j++)
            if((*this).mine[i][j])
                (*this).local_campo[i][j] = 12;
    (*this).local_campo[x][y] = 13;
}

void ccampo::freeCell(int x, int y)
{
    int xmin,xmax,ymin,ymax;

    (*this).local_campo[x][y] = (*this).mine_count(x,y);

    if((*this).local_campo[x][y] == 0)
    {
        if (x == 0)
            xmin = 0;
        else
            xmin = x - 1;
        if (x == ((*this).width - 1))
            xmax = x;
        else
            xmax = x + 1;

        if (y == 0)
            ymin = 0;
        else
            ymin = y - 1;
        if (y == ((*this).height - 1))
            ymax = y;
        else
            ymax = y + 1;

        for(int i = xmin; i <= xmax; i++)
            for(int j = ymin; j <= ymax; j++)
                if((x != i) || (y != j))
                {
                    if((*this).local_campo[i][j] > 8)
                        (*this).freeCell(i,j);
                }
    }
}

SDL_Surface* ccampo::load_image(std::string filename)
{
    //Temporary storage for the image that's loaded
    SDL_Surface* loadedImage = NULL;

    //The optimized image that will be used
    SDL_Surface* optimizedImage = NULL;

    //Load the image
    loadedImage = SDL_LoadBMP( filename.c_str() );

    //If nothing went wrong in loading the image
    if( loadedImage != NULL )
    {
        //Create an optimized image
        optimizedImage = SDL_DisplayFormat( loadedImage );

        //Free the old image
        SDL_FreeSurface( loadedImage );
    }

    //Return the optimized image
    return optimizedImage;
}

int main ( int argc, char** argv )
{

    ccampo campominato(10,20,20);


//    // initialize SDL video
//    if ( SDL_Init( SDL_INIT_VIDEO ) < 0 )
//    {
//        printf( "Unable to init SDL: %s\n", SDL_GetError() );
//        return 4;
//    }

//    // make sure SDL cleans up before exit
//    atexit(SDL_Quit);

    // create a new window
//    SDL_Surface* screen = SDL_SetVideoMode(campominato.get_width() * 16, campominato.get_height() * 16, 16,
//                                           SDL_HWSURFACE|SDL_DOUBLEBUF);
//    if ( !screen )
//    {
//        printf("Errore di inizializzazione del form: %s\n", SDL_GetError());
//        return 1;
//    }


    // program main loop
    bool done = false;
    while (!done)
    {
        // message processing loop
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            // check for messages
            switch (event.type)
            {
                // exit if the window is closed
            case SDL_QUIT:
                done = true;
                break;

                // check for keypresses
            case SDL_KEYDOWN:
                {
                    // exit if ESCAPE is pressed
                    if (event.key.keysym.sym == SDLK_ESCAPE)
                        done = true;
                    break;
                }
            case SDL_MOUSEBUTTONDOWN:
                if ((event.button.x <= (campominato.get_width() * 16)) && (event.button.y <= (campominato.get_height() * 16)))
                    campominato.mousedown_event(event.button.x / 16,event.button.y / 16,event.button.button);
                break;
            case SDL_MOUSEBUTTONUP:
                 if ((event.button.x <= (campominato.get_width() * 16)) && (event.button.y <= (campominato.get_height() * 16)))
                    campominato.mouseup_event(event.button.x / 16,event.button.y / 16,event.button.button);
                break;
            } // end switch
        } // end of message processing

//        // DRAWING STARTS HERE
//
        campominato.draw_campo(screen,0,0);
    } // end main loop


    // all is well ;)
    //printf("Exited cleanly\n");
    return 0;
}
