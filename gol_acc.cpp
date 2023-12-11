#include <algorithm>
#include <vector>
#include <iostream>
#include <string>
#include <cstring>
#include "timer.h"

#define RESTRICT restrict

typedef unsigned char byte;

class world {
public:
    world( int x, int y ) : _wid( x ), _hei( y ) {
        int s = _wid * _hei * sizeof( byte );
        _cells = new byte[s];
        memset( _cells, 0, s );
     #pragma acc enter data copyin(this)
     #pragma acc enter data create(_cells[0:s])
    }
    ~world() {
    #pragma acc exit data delete(_cells)
    #pragma acc exit data delete(this)
        delete [] _cells;
    }
    int wid() const {
        return _wid;
    }
    int hei() const {
        return _hei;
    }
    #pragma acc routine seq
    byte at( int x, int y ) const {
        return _cells[x + y * _wid];
    }
    #pragma acc routine seq
    void set( int x, int y, byte c ) {
        _cells[x + y * _wid] = c;
    }
    #pragma acc routine seq
    void swap( world* w ) {
        memcpy( _cells, w->_cells, _wid * _hei * sizeof( byte ) );
    }
private:
    int _wid, _hei;
    byte* _cells;
};

class rule {
public:
    rule( world* w ) : wrd( w ) {
        wid = wrd->wid();
        hei = wrd->hei();
        wrdT = new world( wid, hei );
     #pragma acc enter data copyin(this)
    }
    ~rule() {
        #pragma acc exit data delete(this)
        if( wrdT ) delete wrdT;
    }
    //potential point for optimization due to the hei/wid variables.
    bool hasLivingCells() {
	bool anyLivingCells = false;
       #pragma acc data present(n) 
       #pragma acc parallel loop \
			     collapse(1)\
			     /*device_type(nvidia)*/\
 	                     gang \
			     /*num_workers(4)*/ \
			     vector \
			     vector_length(128) \
                            /* tile(4,4)*/   
       	for( int y = 0; y < hei; y++ )
             #pragma acc loop vector  
             for( int x = 0; x < wid; x++ )
                if( wrd->at( x, y ) ) anyLivingCells = true;
        	//std::cout << "*** All cells are dead!!! ***\n\n";
        if(anyLivingCells) return true;
	else 
	{
	    std::cout << "*** All cells are dead!!! ***\n\n";
	    return false;
	}
	
    }
    void swapWrds() {
        wrd->swap( wrdT );
    }
    void setRuleB( std::vector<int>& birth ) {
        _birth = birth;
    }
    void setRuleS( std::vector<int>& stay ) {
        _stay = stay;
    }
    //another point of optimization
    //#pragma acc routine seq
    void applyRules() {
        int n; 
	#pragma acc data copy(n[0:wid])
	//#pragma acc kernels
	#pragma acc parallel \
			loop \
                        collapse(1)\
                        gang/* num_gangs(4)*/\
			/*num_workers(4)*/\
		        vector_length(64)
        for( int y = 0; y < hei; y++ ) { 
	    #pragma acc loop vector  
            for( int x = 0; x < wid; x++ ) {
                n = neighbours( x, y );
                if( wrd->at( x, y ) ) {
                    wrdT->set( x, y, inStay( n ) ? 1 : 0 );
                } else {
                    wrdT->set( x, y, inBirth( n ) ? 1 : 0 );
                }
            }
        }
	
    }
    //potentially not a good candidate due to the short lengths of the iterations
private:
    #pragma acc routine seq
    int neighbours( int xx, int yy ) {
        int n = 0, nx, ny;
        for( int y = -1; y < 2; y++ ) {
            for( int x = -1; x < 2; x++ ) {
                if( !x && !y ) continue;
                nx = ( wid + xx + x ) % wid;
                ny = ( hei + yy + y ) % hei;
                n += wrd->at( nx, ny ) > 0 ? 1 : 0;
            }
        }
        return n;
    }
    //#pragma acc routine seq
    bool inStay( int n ) {
        return( _stay.end() != find( _stay.begin(), _stay.end(), n ) );
    }
    //#pragma acc routine seq
    bool inBirth( int n ) {
        return( _birth.end() != find( _birth.begin(), _birth.end(), n ) );
    }
    int wid, hei;
    world *wrd, *wrdT;
    std::vector<int> _stay, _birth;
};
class cellular {
public:
    cellular( int w, int h ) : rl( 0 ) {
        wrd = new world( w, h );
	//#pragma acc enter data copyin(this)
    }
    ~cellular() {
	//#pragma acc exit data delete(this)
        if( rl )  delete rl;
        delete wrd;
    }
    //customized to allow predetermined generations
    void start( int r, int g ) {
        rl = new rule( wrd );
        gen = 1;
        std::vector<int> t;
        switch( r ) {
            case 1: // conway
                t.push_back( 2 ); t.push_back( 3 ); rl->setRuleS( t );
                t.clear(); t.push_back( 3 ); rl->setRuleB( t );
                break;
            case 2: // amoeba
                t.push_back( 1 ); t.push_back( 3 ); t.push_back( 5 ); t.push_back( 8 ); rl->setRuleS( t );
                t.clear(); t.push_back( 3 ); t.push_back( 5 ); t.push_back( 7 ); rl->setRuleB( t );
                break;
            case 3: // life34
                t.push_back( 3 ); t.push_back( 4 ); rl->setRuleS( t );
                rl->setRuleB( t );
                break;
            case 4: // maze
                t.push_back( 1 ); t.push_back( 2 ); t.push_back( 3 ); t.push_back( 4 ); t.push_back( 5 ); rl->setRuleS( t );
                t.clear(); t.push_back( 3 ); rl->setRuleB( t );
                break;
        }

        /* just for test - should read from a file */
        /* GLIDER */
        wrd->set( 6, 1, 1 ); wrd->set( 7, 2, 1 );
        wrd->set( 5, 3, 1 ); wrd->set( 6, 3, 1 );
        wrd->set( 7, 3, 1 );
        /* BLINKER */
        wrd->set( 1, 3, 1 ); wrd->set( 2, 3, 1 );
        wrd->set( 3, 3, 1 );
        /******************************************/
        generation(g);
    }
private:
    //potential candidate (poor) for optimization
    //should investigate a way to record runtime for each iteration.
    //#pragma acc routine seq
    void display() {
        system( "cls" );
        int wid = wrd->wid(),
                hei = wrd->hei();
        std::cout << "+" << std::string( wid, '-' ) << "+\n";
        //#pragma acc kernels 
        for( int y = 0; y < hei; y++ ) {
            std::cout << "|";
            for( int x = 0; x < wid; x++ ) {
                if( wrd->at( x, y ) ) std::cout << "#";
                else std::cout << ".";
            }
            std::cout << "|\n";
        }
        std::cout << "+" << std::string( wid, '-' ) << "+\n";
        std::cout << "Generation: " << gen << "\n\n";//Press [RETURN] for the next generation...";

    }

    //Included parameter to enable generation control
    void generation(int g) {
        double t_begin  = 0;
	double t_ave    = 0;
        double ave_time = 0;
        TimerType t0 = getTimeStamp();
        do {
            display();
	    TimerType t1 = getTimeStamp();
            rl->applyRules();
            rl->swapWrds();
	    TimerType  t2 = getTimeStamp();
            double elapsed_t = getElapsedTime(t1,t2);
            ave_time += elapsed_t;
            gen++;
        }
        while ( gen < g );
        TimerType t1 = getTimeStamp();
        t_begin = getElapsedTime(t0,t1)*1000;
        double average_runtime = (ave_time/g)*1000;
        std::cout << "Runtime for " << g << " generations: " << t_begin << "ms" << "\n\n";
	std::cout << "Average runtime per generation: " << average_runtime << "ms" << "\n\n"; 
    }
    rule* rl;
    world* wrd;
    int gen;
};

void help(const char* prg)
{
    if (prg) fprintf(stderr,"%s:\n", prg);
    fprintf(stderr,"\t--help    | -h       : Print help message.\n");
    fprintf(stderr,"\t--nheight | -nh      : height of world.\n");
    fprintf(stderr,"\t--ngen    | -g       : # of generations to print (10).\n");
    fprintf(stderr,"\t--nwidth  | -w       : with of world\n");
    fprintf(stderr,"\t--float   | -f       : Use 32-bit floats.\n");
    fprintf(stderr,"\t--double  | -d       : Use 64-bit doubles. (default)\n");
}

int main( int argc, char* argv[] ) {
    //default cellular automata: in this case, Game of Life
    int automata = 1;

    //the default number of generations
    int g = 10;

    //default world size
    int height = 12;
    int width = 20;

    for (int i = 1; i < argc; ++i) {
#define check_index(i, str) \
    if ((i) >= argc) \
    {fprintf(stderr, "Missing 2nd argument for %s\n", str);help(argv[0]);return 1; }

        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
        {
            help(argv[0]);
            return 0;
        }
        else if (strcmp(argv[i], "--nheight") == 0 || strcmp(argv[i], "-nh") == 0)
        {
            check_index(i + 1, "--nheight|-nh") ;
            i++;
            if (not(isdigit(*argv[i]))) {
                fprintf(stderr, "Invalid value for option \"--ncells\" %s\n", argv[i]);help(argv[0]);return 1;
            }
            height = atoi(argv[i]);
        }
        else if (strcmp(argv[i], "--nwidth") == 0 || strcmp(argv[i], "-w") == 0) {
            check_index(i + 1, "--nwidth|-w") ;
            i++;
            if (not(isdigit(*argv[i])))
            {fprintf(stderr, "Invalid value for option \"--ncells\" %s\n", argv[i]);help(argv[0]);return 1;}
            width = atoi(argv[i]);
        } else if (strcmp(argv[i], "--ngenerations") == 0 || strcmp(argv[i], "-g") == 0) {
            check_index(i + 1, "--ngenerations|-g") ;
            i++;
            if (not(isdigit(*argv[i])))
            {fprintf(stderr, "Invalid value for option \"--ngenerations\" %s\n", argv[i]);help(argv[0]);return 1;}
            g = atoi(argv[i]);
        } else
        {
            fprintf(stderr, "Unknown option %s\n", argv[i]);
            help(argv[0]);
            return 1;
        }
    }

    cellular c( width, height );
    c.start( automata,g );
    return 0;
}
