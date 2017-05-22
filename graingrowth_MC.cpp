//mpirun -n 4 ./parallel_MC.out --nonstop 2 voronmc.000.dat 600 600 0 4 2 704 489 4 3
//8,472,302,3,1,1
// graingrowth.hpp
// Algorithms for 2D and 3D isotropic Monte Carlo grain growth
// Questions/comments to yixuan.john.tan@gmail.com (Yixuan Tan)
// Cancel periodic b.c. inside the MMSP.grid.hpp, add:
/*---------cancel periodic boundary condition-----------
        if(x0[i] == g0[i]){
				  send_min[i] = send_max[i];
          recv_min[i] = recv_max[i];
        }
---------cancel periodic boundary condition-----------*/ 
// When checking neighour during flipping, add checking: (Note the >=)
/*
	            if(r[0]<g0(*(ss->grid), 0) || r[0]>=g1(*(ss->grid), 0) || r[1]<g0(*(ss->grid), 1) || r[1]>=g1(*(ss->grid), 1) 
                 || r[2]<g0(*(ss->grid), 2) || r[2]>=g1(*(ss->grid), 2))
*/

// To avoid discontinuous grian structure, make sure ghostswap(grid) before starting loop in update(); Why do this? Not clear yet, maybe file output part changes the ghost part in grid? 

#ifndef GRAINGROWTH_UPDATE
#define GRAINGROWTH_UPDATE
#include <iomanip>
#include <fstream>
#include <vector>
#include <cmath>
#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <pthread.h>
#include <stdexcept>
#include <sstream>
#include "rdtsc.h"
#include"graingrowth_MC.hpp"
#include"MMSP.hpp"
#include"tessellate.hpp"
#include"output.cpp"

int grad_pos_start = 100;
int grad_pos_end = 150;
int mid_check = (grad_pos_end + grad_pos_start) / 2;
double domainLen = 10.0;
double position_stack[241];
double pointtemp_stack[241];

/* ------- Al-Cu alloy film
double lambda = 3.75e-3; //This is fixed from Monte Carlo simulation, so do not change it.  here 10 um is the domain size, so each pixel is 10 nm, all length unit should be with um.
double L_initial = 30.0e-3; // initially 30 nm diameter
double L0 = 30.0e-3;
double K1 = 0.6200;
double n1 = 0.5130;
double Q = 1.2552e5; //fitted from Mcbee, William C., and John A. McComb. "Grain growth in thin aluminum-4% copper alloy films." Thin Solid Films 30.1 (1975): 137-143.
double n = 1.0/0.18; 
double K_ = 2.4670; // length in um, time in second fitted from Mcbee, William C., and John A. McComb. "Grain growth in thin aluminum-4% copper alloy films." Thin Solid Films 30.1 (1975): 137-143.
double R = 8.314;
*/

// ---------Cu film
double lambda = 1.1/1*1.0e-3;  //length unit is in mm, each pixel is 0.275 um
double L_initial = 1.1e-3; 
double L0 = 1.1e-3; 
double K1 = 0.6263;
double m = 2.0697;
double n1 = 1.0/m;
double Q = 146000; //fitted from Gangulee, A. ”Structure of electroplated and vapordeposited copper films. III. Recrystallization and grain growth.” Journal of Applied Physics 45.9 (1974): 3749-3756.
double n = 2; 
double K_ = 94.3; //fitted from Gangulee, A. ”Structure of electroplated and vapordeposited copper films. III. Recrystallization and grain growth.” Journal of Applied Physics 45.9 (1974): 3749-3756.
double R = 8.314;

// grid point dimension
int dim_x = 500; 
int dim_y = 200; 
int dim_z = 700; 

namespace MMSP { 
	template <int dim> bool OutsideDomainCheck(MMSP::grid<dim, unsigned long>& grid, vector<int>* x);

  	template <int dim> void UpdateLocalTmp(MMSP::grid<dim, unsigned long>& grid, double position[], double pointtemp[], double maxTemp, double minTemp){
		//int rank = MPI::COMM_WORLD.Get_rank();
		//if(rank == 0) for(int tt = 0; tt < 241; tt++) std::cout << pointtemp[tt] << std::endl;

		std::vector<int> tempFullSpace(dim_x);
		int j = 0, len = 241;
		for(int i = 1; i < len; i++) {
			double prev = pointtemp[i-1];
			double slope = (pointtemp[i] - pointtemp[i-1]) / (position[i] / domainLen  - position[i-1] / domainLen);
			while(j < position[i] / domainLen * dim_x) {
				tempFullSpace[j] = pointtemp[i-1] + ((double)j / dim_x - position[i-1] / domainLen) * slope;
				j++;
			}
		}
		tempFullSpace.back() = pointtemp[len - 1];
		int rank = MPI::COMM_WORLD.Get_rank();
		
		if(rank == 3) {
			std::cout << "\n started " << std::endl;
			for(auto& tt : tempFullSpace) {
				std::cout << std::setw(10) << tt;
			}
			std::cout << "done \n" << std::endl;
		}
		
		maxTemp = 773.0;
		minTemp = 473.0;
		vector<int> coords (dim,0);
		if(dim==2){
		    for(int codx=x0(grid, 0); codx < x1(grid, 0); codx++) {
				coords[0] = codx;
		    	for(int cody=x0(grid, 1); cody < x1(grid, 1); cody++){
		        	coords[1] = cody;
			    	grid.AccessToTmp(coords) = tempFullSpace[codx]; //(codx < 100 ? 500 : tempFullSpace[codx]);
		        	/*
		        	if (codx >= grad_pos_start && codx <= grad_pos_end) {
		        		grid.AccessToTmp(coords) = maxTemp - (maxTemp - minTemp) / (grad_pos_end - grad_pos_start) * (codx - grad_pos_start);
		        	}
		        	else grid.AccessToTmp(coords) = 273.0; 
			    	*/
			    } 
			    //std::cout << "at codx = " << codx << "  " << tempFullSpace[codx] << std::endl;
		    }
		} else if(dim==3){
			for(int codx=x0(grid, 0); codx < x1(grid, 0); codx++) {
				for(int cody=x0(grid, 1); cody < x1(grid, 1); cody++) {
					for(int codz=x0(grid, 2); codz < x1(grid, 2); codz++){
						coords[0] = codx;
						coords[1] = cody;
						coords[2] = codz;
					}
				}
			}
		}
	}

template <int dim>
unsigned long generate(MMSP::grid<dim,unsigned long >*& grid, int seeds, int nthreads)
{
	#if (defined CCNI) && (!defined MPI_VERSION)
	std::cerr<<"Error: MPI is required for CCNI."<<std::endl;
	exit(1);
	#endif
	#ifdef MPI_VERSION
	int np = MPI::COMM_WORLD.Get_size();
	#endif

	unsigned long timer=0;
	if (dim == 2) {
		int number_of_fields(seeds);
		if (number_of_fields==0) number_of_fields = static_cast<int>(float(dim_x*dim_y)/(M_PI*.5*.5)); /* average grain is a disk of radius XXX
, XXX cannot be smaller than 0.1, or BGQ will abort.*/
		#ifdef MPI_VERSION
		while (number_of_fields % np) --number_of_fields; 
		#endif
		grid = new MMSP::grid<dim,unsigned long>(0, 0, dim_x, 0, dim_y);

		#ifdef MPI_VERSION
		number_of_fields /= np;
		#endif

		#if (!defined MPI_VERSION) && ((defined CCNI) || (defined BGQ))
		std::cerr<<"Error: CCNI requires MPI."<<std::endl;
		std::exit(1);
		#endif
		timer = tessellate<dim,unsigned long>(*grid, number_of_fields, nthreads);
		#ifdef MPI_VERSION
		MPI::COMM_WORLD.Barrier();
		#endif
	} else if (dim == 3) {
		int number_of_fields(seeds);
		if (number_of_fields==0) number_of_fields = static_cast<int>(float(dim_x*dim_y*dim_z)/(4./3*M_PI*0.5*0.5*0.5)); // Average grain is a sphere of radius 10 voxels
		#ifdef MPI_VERSION
		while (number_of_fields % np) --number_of_fields;
		#endif
		grid = new MMSP::grid<dim,unsigned long>(0, 0, dim_x, 0, dim_y, 0, dim_z);

		#ifdef MPI_VERSION
		number_of_fields /= np;
		#endif

//		timer = tessellate<dim,unsigned long >(*grid, number_of_fields, nthreads);
		#ifdef MPI_VERSION
		MPI::COMM_WORLD.Barrier();
		#endif
	}
/*------------------Initial tmc----------------------*/
	double tmc_initial = 0;//pow((L_initial/lambda-1)/K1,1.0/n1);
  vector<int> coords (dim,0);
  if(dim==2){
    for(int codx=x0(*grid, 0); codx < x1(*grid, 0); codx++) 
      for(int cody=x0(*grid, 1); cody < x1(*grid, 1); cody++){
        coords[0] = codx;
        coords[1] = cody;
//        (*grid)(coords) = coords[0]*dim_y + coords[1] + 1;// grain id start from 1
        (*grid).AccessToTmc(coords) = tmc_initial;
      }
  }
  else if(dim==3){
    for(int codx=x0(*grid, 0); codx < x1(*grid, 0); codx++) 
      for(int cody=x0(*grid, 1); cody < x1(*grid, 1); cody++) 
        for(int codz=x0(*grid, 2); codz < x1(*grid, 2); codz++){
          coords[0] = codx;
          coords[1] = cody;
          coords[2] = codz;
          (*grid)(coords) = coords[0]*dim_y*dim_z + coords[1]*dim_z + coords[2] + 1;// grain id start from 1
          (*grid).AccessToTmc(coords) = tmc_initial;
        }
  }
  //double temp[2] = {673,723};
  //    UpdateLocalTmp((*grid), 0, 0, 0.0);  
/*---------------------------------------------------*/
	return timer;
}

unsigned long generate(int dim, char* filename, int seeds, int nthreads)
{
	#if (defined CCNI) && (!defined MPI_VERSION)
	std::cerr<<"Error: MPI is required for CCNI."<<std::endl;
	exit(1);
	#endif
	int rank=0;
	#ifdef MPI_VERSION
	rank = MPI::COMM_WORLD.Get_rank();
	#endif

	unsigned long timer = 0;
/*------------------Initial tmc----------------------*/
	double tmc_initial = 0;//pow((L_initial/lambda-1)/K1,1.0/n1);
  vector<int> coords (dim,0);
/*---------------------------------------------------*/

	if (dim == 2) {
		MMSP::grid<2,unsigned long>* grid2=NULL;
		timer = generate<2>(grid2,seeds,nthreads);
		assert(grid2!=NULL);
		#ifdef BGQ
		output_bgq(*grid2, filename);
		#else
		output(*grid2, filename);
		#endif
		#ifndef SILENT
		if (rank==0) std::cout<<"Wrote initial file to "<<filename<<"."<<std::endl;
		#endif
/*------------------Initial tmc----------------------*/
    for(int codx=x0(*grid2, 0); codx < x1(*grid2, 0); codx++) 
      for(int cody=x0(*grid2, 1); cody < x1(*grid2, 1); cody++){
        coords[0] = codx;
        coords[1] = cody;
        (*grid2).AccessToTmc(coords) = tmc_initial;
//        (*grid2).AccessToTmp(coords) = 1.0e6; //set the initial temp to be large enough such that initial physical time = 0.
      }
/*---------------------------------------------------*/
	}

	if (dim == 3) {
		MMSP::grid<3,unsigned long>* grid3=NULL;
		timer = generate<3>(grid3,seeds,nthreads);
		assert(grid3!=NULL);
		#ifdef BGQ
		output_bgq(*grid3, filename);
		#else
		output(*grid3, filename);
		#endif
		#ifndef SILENT
		if (rank==0) std::cout<<"Wrote initial file to "<<filename<<"."<<std::endl;
		#endif
/*------------------Initial tmc----------------------*/
    for(int codx=x0(*grid3, 0); codx < x1(*grid3, 0); codx++) 
      for(int cody=x0(*grid3, 1); cody < x1(*grid3, 1); cody++) 
        for(int codz=x0(*grid3, 2); codz < x1(*grid3, 2); codz++){
          coords[0] = codx;
          coords[1] = cody;
          coords[2] = codz;
          (*grid3).AccessToTmc(coords) = tmc_initial;
          (*grid3).AccessToTmp(coords) = 1.0e6;
        }
/*---------------------------------------------------*/
	}

	return timer;
}

template <int dim>
unsigned long generate(MMSP::grid<dim,unsigned long>*& grid, const char* filename)
{
	#if (defined CCNI) && (!defined MPI_VERSION)
	std::cerr<<"Error: MPI is required for CCNI."<<std::endl;
	exit(1);
	#endif
	#ifdef MPI_VERSION
	int np = MPI::COMM_WORLD.Get_size();
	#endif

	unsigned long timer=0;
/*------------------Initial tmc----------------------*/
	double tmc_initial = 0;//pow((L_initial/lambda-1)/K1,1.0/n1);
  vector<int> coords (dim,0);
/*---------------------------------------------------*/

	if (dim == 2) {
		grid = new MMSP::grid<dim,unsigned long>(0, 0, dim_x, 0, dim_y);
		(*grid).input(filename, 1, false);
		#ifdef MPI_VERSION
		MPI::COMM_WORLD.Barrier();
		#endif
/*------------------Initial tmc----------------------*/
    for(int codx=x0(*grid, 0); codx < x1(*grid, 0); codx++) 
      for(int cody=x0(*grid, 1); cody < x1(*grid, 1); cody++){
        coords[0] = codx;
        coords[1] = cody;
        (*grid).AccessToTmc(coords) = tmc_initial;
      }
/*---------------------------------------------------*/
	} else if (dim == 3) {
		grid = new MMSP::grid<dim,unsigned long>(0, 0, dim_x, 0, dim_y, 0, dim_z);
		(*grid).input(filename, 1, false);
		#ifdef MPI_VERSION
		MPI::COMM_WORLD.Barrier();
		#endif
/*------------------Initial tmc----------------------*/
    for(int codx=x0(*grid, 0); codx < x1(*grid, 0); codx++) 
      for(int cody=x0(*grid, 1); cody < x1(*grid, 1); cody++) 
        for(int codz=x0(*grid, 2); codz < x1(*grid, 2); codz++){
          coords[0] = codx;
          coords[1] = cody;
          coords[2] = codz;
          (*grid).AccessToTmc(coords) = tmc_initial;
        }
/*---------------------------------------------------*/
	}
	//double temp[2] = {673,723};
	//	UpdateLocalTmp((*grid), 0, 0, 0.0);  
	return timer;
}

template <int dim>
unsigned long growthexperiment(MMSP::grid<dim,unsigned long >*& grid, const char* filename)
{
	#if (defined CCNI) && (!defined MPI_VERSION)
	std::cerr<<"Error: MPI is required for CCNI."<<std::endl;
	exit(1);
	#endif
	#ifdef MPI_VERSION
	int np = MPI::COMM_WORLD.Get_size();
	#endif
	unsigned long timer=0;
	if (dim == 2) {
		grid = new MMSP::grid<dim,unsigned long>(0, 0, dim_x, 0, dim_y);
		#ifdef MPI_VERSION
		MPI::COMM_WORLD.Barrier();
		#endif
	} else if (dim == 3) {
		grid = new MMSP::grid<dim,unsigned long>(0, 0, dim_x, 0, dim_y, 0, dim_z);
		#ifdef MPI_VERSION
		MPI::COMM_WORLD.Barrier();
		#endif
	}

/*---------------read grain ID------------------*/
	std::ifstream idtxt(filename, std::ios::in);
	if (!idtxt) {
		std::cerr << "File idtxt error: could not open ";
		std::cerr << filename << "." << std::endl;
	  exit(-1);
  }

  int* ids = new int[dim_x*dim_y];
  int grainID;
  int count = 0;
  int fields = 1;
  while(idtxt>>grainID){
    ids[count] = grainID;
    ++count; 
    fields = (grainID>fields)?grainID:fields;
  }
	MPI::COMM_WORLD.Barrier();
  idtxt.close();
/*----------------------------------------------*/

/*------------------Initial tmc----------------------*/
  double tmc_initial = 0;//pow((L_initial/lambda-1)/K1,1.0/n1);
  vector<int> coords (dim,0);

  if(dim==2){
    for(int codx=x0(*grid, 0); codx < x1(*grid, 0); codx++) {
      for(int cody=x0(*grid, 1); cody < x1(*grid, 1); cody++){
        coords[0] = codx;
        coords[1] = cody;
        (*grid).AccessToTmc(coords) = tmc_initial;
        (*grid).AccessToTmp(coords) = 1.0e6; //set the initial temp to be large enough such that initial physical time = 0.
        (*grid)(coords) = ids[dim_y*codx+cody];
      }
    }  
  }
  else if(dim==3){
    for(int codx=x0(*grid, 0); codx < x1(*grid, 0); codx++) 
      for(int cody=x0(*grid, 1); cody < x1(*grid, 1); cody++)
        for(int codz=x0(*grid, 2); codz < x1(*grid, 2); codz++){
          coords[0] = codx;
          coords[1] = cody;
          coords[2] = codz;
          (*grid).AccessToTmc(coords) = tmc_initial;
          (*grid).AccessToTmp(coords) = 1.0e6; //set the initial temp to be large enough such that initial physical time = 0.
          (*grid)(coords) = ids[dim_y*dim_z*codx+dim_z*cody+codz];
        }
  }
/*--------------------------------------------------*/
  MPI::COMM_WORLD.Barrier();
  delete [] ids;
  ids = NULL;
  return timer;
}

int LargeNearestInteger(int a, int b){
  if(a%b==0) return a/b;
  else return a/b+1;
}

template <int dim> struct flip_index {
	MMSP::grid<dim, unsigned long>* grid;
  int num_of_cells_in_thread;
	int sublattice;
  int num_of_points_to_flip;
  int cell_coord[dim];
  int lattice_cells_each_dimension[dim];
  double Pdenominator;
};

template <int dim> void* flip_index_helper( void* s )
{
  double cos_tolerance=0.999; //cos^-1(0.999)=2.563 deg   cos^-1(0.9999)=0.8103 deg
  int rank = MPI::COMM_WORLD.Get_rank();
  srand((unsigned)time(NULL) + (unsigned)rank); /* seed random number generator */
	flip_index<dim>* ss = static_cast<flip_index<dim>*>(s);
  double kT=0.0;
	vector<int> x (dim,0);
  int first_cell_start_coordinates[dim];
  for(int kk=0; kk<dim; kk++) first_cell_start_coordinates[kk] = x0(*(ss->grid), kk);
  for(int i=0; i<dim; i++){
    if(x0(*(ss->grid), i)%2!=0) first_cell_start_coordinates[i]--;
  }
  int cell_coords_selected[dim];
	for (int hh=0; hh<ss->num_of_points_to_flip; hh++) {
	  // choose a random cell to flip
    int cell_numbering_in_thread = rand()%(ss->num_of_cells_in_thread); //choose a cell to flip, from 0 to num_of_cells_in_thread-1
    if(dim==2){
      cell_coords_selected[dim-1]=((ss->cell_coord)[dim-1]+cell_numbering_in_thread)%(ss->lattice_cells_each_dimension)[dim-1];//1-indexed
      cell_coords_selected[0]=(ss->cell_coord)[0]+(((ss->cell_coord)[dim-1]+cell_numbering_in_thread)/(ss->lattice_cells_each_dimension)[dim-1]);
    }else if(dim==3){
      cell_coords_selected[dim-1]=((ss->cell_coord)[dim-1]+cell_numbering_in_thread)%(ss->lattice_cells_each_dimension)[dim-1];//1-indexed
      cell_coords_selected[1]=(  (ss->cell_coord)[1]+ ((ss->cell_coord)[dim-1]+cell_numbering_in_thread)/(ss->lattice_cells_each_dimension)[dim-1]  )%(ss->lattice_cells_each_dimension)[1];
      cell_coords_selected[0]=(ss->cell_coord)[0]+ ( (ss->cell_coord)[1] + ((ss->cell_coord)[dim-1]+cell_numbering_in_thread)/(ss->lattice_cells_each_dimension)[dim-1] ) /(ss->lattice_cells_each_dimension)[1];
    }
    for(int i=0; i<dim; i++){
      x[i]=first_cell_start_coordinates[i]+2*cell_coords_selected[i];
    }
    if(dim==2){
      switch(ss->sublattice){
        case 0:break;// 0,0
        case 1:x[1]++; break; //0,1
        case 2:x[0]++; break; //1,0
        case 3:x[0]++; x[1]++; break; //1,1
      }
    }else if(dim==3){
      switch(ss->sublattice){
        case 0:break;// 0,0,0
        case 1:x[2]++; break; //0,0,1
        case 2:x[1]++; break; //0,1,0
        case 3:x[2]++; x[1]++; break; //0,1,1
        case 4:x[0]++; break; //1,0,0
        case 5:x[2]++; x[0]++; break; //1,0,1
        case 6:x[1]++; x[0]++; break; //1,1,0
        case 7:x[2]++; x[1]++; x[0]++; //1,1,1
      }
    }

    bool site_out_of_domain = false;
    if(OutsideDomainCheck<dim>(*(ss->grid), &x)){
      hh--;
      continue; //continue the int hh loop
    }

    int rank = MPI::COMM_WORLD.Get_rank();

    double temperature=(*(ss->grid)).AccessToTmp(x);
    double t_mcs = (*(ss->grid)).AccessToTmc(x);
    double Pnumerator = exp(-Q/R/temperature)/pow( pow(L_initial/lambda, m) + K1 * t_mcs, (double)n/m - 1.0) ; //exp(-Q/R/temperature)/pow(t_mcs,n1-1)/pow((1+K1*pow(t_mcs,n1)),n-1);
    double site_selection_probability = Pnumerator/ss->Pdenominator;
	  double rd = double(rand())/double(RAND_MAX);
    if(rd>site_selection_probability){
//      hh--;// no need to guarantee that N times selection is performed in a configurational MC step, so hh is ony need to be the same as in uniform temp case for the highest temp site
      continue;//this site wont be selected
    }

	unsigned long spin1 = (*(ss->grid))(x);
	// determine neighboring spins
    vector<int> r(dim,0);
    std::vector<unsigned long> neighbors;
    neighbors.clear();
    int number_of_same_neighours = 0;
    if(dim==2){
      for (int i=-1; i<=1; i++) {
        for (int j=-1; j<=1; j++) {
          if(!(i==0 && j==0)){
            r[0] = x[0] + i;
            r[1] = x[1] + j;
            if(r[0]<g0(*(ss->grid), 0) || r[0]>=g1(*(ss->grid), 0) || r[1]<g0(*(ss->grid), 1) || r[1]>=g1(*(ss->grid), 1) )
              continue;// neighbour outside the global boundary, skip it. 

            unsigned long spin = (*(ss->grid))(r);
            neighbors.push_back(spin);
            if(spin==spin1) 
              number_of_same_neighours++;
          }
        }
      }
    }else if(dim==3){
		  for (int i=-1; i<=1; i++){
			  for (int j=-1; j<=1; j++){
			    for (int k=-1; k<=1; k++) {
            if(!(i==0 && j==0 && k==0)){
				      r[0] = x[0] + i;
				      r[1] = x[1] + j;
				      r[2] = x[2] + k;

/*----------cancel periodic b.c.-----------*/
	            if(r[0]<g0(*(ss->grid), 0) || r[0]>=g1(*(ss->grid), 0) || r[1]<g0(*(ss->grid), 1) || r[1]>=g1(*(ss->grid), 1) 
                 || r[2]<g0(*(ss->grid), 2) || r[2]>=g1(*(ss->grid), 2))
                continue;// neighbour outside the global boundary, skip it. 
/*----------cancel periodic b.c.-----------*/

				      unsigned long spin = (*(ss->grid))(r);
              neighbors.push_back(spin);
              if(spin==spin1) 
                number_of_same_neighours++;
            }
			    }
        }
		  }
    }
    
    //check if inside a grain
    if(number_of_same_neighours==neighbors.size()){//inside a grain
      continue;//continue for
    }
    //     choose a random neighbor spin
    unsigned long spin2 = neighbors[rand()%neighbors.size()];
		// choose a random spin from Q states
 //       int spin2 = rand()%200;
		if (spin1!=spin2){
			// compute energy change
			double dE = 0.0;
      if(dim==2){
			  for (int i=-1; i<=1; i++){
				  for (int j=-1; j<=1; j++){
            if(!(i==0 && j==0)){
  					  r[0] = x[0] + i;
	  				  r[1] = x[1] + j;

	            if(r[0]<g0(*(ss->grid), 0) || r[0]>=g1(*(ss->grid), 0) || r[1]<g0(*(ss->grid), 1) || r[1]>=g1(*(ss->grid), 1) )
              continue;// neighbour outside the global boundary, skip it. 
    				  unsigned long spin = (*(ss->grid))(r);
				  dE += 1.0/2*((spin!=spin2)-(spin!=spin1));

            }// if(!(i==0 && j==0))
				  }
        }
      }
      if(dim==3){
			  for (int i=-1; i<=1; i++){ 
				  for (int j=-1; j<=1; j++){ 
    	      for (int k=-1; k<=1; k++){ 
              if(!(i==0 && j==0 && k==0)){
					      r[0] = x[0] + i;
					      r[1] = x[1] + j;
					      r[2] = x[2] + k;

/*----------cancel periodic b.c.-----------*/
	            if(r[0]<g0(*(ss->grid), 0) || r[0]>=g1(*(ss->grid), 0) || r[1]<g0(*(ss->grid), 1) || r[1]>=g1(*(ss->grid), 1) 
                 || r[2]<g0(*(ss->grid), 2) || r[2]>=g1(*(ss->grid), 2))
                  continue;// neighbour outside the global boundary, skip it.
/*----------cancel periodic b.c.-----------*/

					      unsigned long spin = (*(ss->grid))(r);
					      dE += 1.0/2*((spin!=spin2)-(spin!=spin1));
              }
				    }
          }
        }
      }
			// attempt a spin flip
  
      double r = double(rand())/double(RAND_MAX);
      if (dim == 2) {
        kT = 0.6634;
      } else if (dim == 3){
        kT = 5.58;
      }
      
      if (dE <= 0.0) {
        (*(ss->grid))(x) = spin2;
      }
      else if (r<exp(-dE/kT)) {
        (*(ss->grid))(x) = spin2;
      }
      /*
      if (dE <= 0.0) {                                                                                                                                                              
        (*(ss->grid))(x) = spin2;                                                                                                                                                  
      }                                                                                                                                                                            
      */
		}
	}
	pthread_exit(0);
	return NULL;
}

template <int dim> bool OutsideDomainCheck(MMSP::grid<dim, unsigned long>& grid, vector<int>* x){
  bool outside_domain=false;
  for(int i=0; i<dim; i++){
    if((*x)[i]<x0(grid, i) || (*x)[i]>x1(grid, i)-1){
      outside_domain=true;
      break;
    }
  }
  return outside_domain;
}

template <int dim> double PdenominatorMax(MMSP::grid<dim, unsigned long>& grid, double& tmc_at_PdenominatorMax, double& tmp_at_PdenominatorMax){
   double Pdenominator_max = 0.0;
   vector<int> coords (dim,0);
   if(dim==2){
       for(int codx=x0(grid, 0); codx < x1(grid, 0); codx++) 
         for(int cody=x0(grid, 1); cody < x1(grid, 1); cody++){
           coords[0] = codx;
           coords[1] = cody;
           double temperature = grid.AccessToTmp(coords);
           double tmc_local = grid.AccessToTmc(coords);
           double Pdenominator = exp(-Q/R/temperature)/pow( pow(L_initial/lambda, m) + K1 * tmc_local, (double)n/m - 1.0); //exp(-Q/R/temperature)/pow(tmc_local,n1-1)/pow((1+K1*pow(tmc_local,n1)),n-1);
           if(Pdenominator > Pdenominator_max){
             Pdenominator_max = Pdenominator;
             tmc_at_PdenominatorMax = grid.AccessToTmc(coords);
             tmp_at_PdenominatorMax = temperature;
           }
         }
   }
   else if(dim==3){
     for(int codx=x0(grid, 0); codx < x1(grid, 0); codx++) 
       for(int cody=x0(grid, 1); cody < x1(grid, 1); cody++) 
         for(int codz=x0(grid, 2); codz < x1(grid, 2); codz++){
           coords[0] = codx;
           coords[1] = cody;
           coords[2] = codz;
           double temperature = grid.AccessToTmp(coords);
           double tmc_local = grid.AccessToTmc(coords);
           double Pdenominator = exp(-Q/R/temperature)/pow( pow(L_initial/lambda, m) + K1 * tmc_local, (double)n/m - 1.0); //exp(-Q/R/temperature)/pow(tmc_local,n1-1)/pow((1+K1*pow(tmc_local,n1)),n-1);                                                                   
           if(Pdenominator > Pdenominator_max){
             Pdenominator_max = Pdenominator;
             tmc_at_PdenominatorMax = grid.AccessToTmc(coords);
             tmp_at_PdenominatorMax = temperature;
           }
        }
   }
   return Pdenominator_max;
}

template <int dim> void UpdateLocalTmc(MMSP::grid<dim, unsigned long>& grid, double t_inc){
   vector<int> coords (dim,0);
   if(dim==2){
       for(int codx=x0(grid, 0); codx < x1(grid, 0); codx++) 
         for(int cody=x0(grid, 1); cody < x1(grid, 1); cody++){
           coords[0] = codx;
           coords[1] = cody;
           double tmc_local = grid.AccessToTmc(coords);
           long double exp_eqn_rhs = pow( pow(lambda, m) * K1 * tmc_local + pow(L_initial, m), (double)n/m) - pow(L0, n); //pow(lambda*(1+K1*pow(tmc_local, n1)), n) - pow(L0, n);
           long double temperature = grid.AccessToTmp(coords);
           exp_eqn_rhs += K_*t_inc*exp(-Q/R/temperature);
           grid.AccessToTmc(coords) = (pow(exp_eqn_rhs + pow(L0, n), (double)m/n) - pow(L_initial, m)) / pow(lambda, m) / K1;//pow(1.0/K1/lambda*pow(exp_eqn_rhs+pow(L0, n),1.0/n)-1.0/K1, 1.0/n1);  
         }
   }
   else if(dim==3){
     for(int codx=x0(grid, 0); codx < x1(grid, 0); codx++)  
         for(int cody=x0(grid, 1); cody < x1(grid, 1); cody++) 
           for(int codz=x0(grid, 2); codz < x1(grid, 2); codz++){
             coords[0] = codx;
             coords[1] = cody;
             coords[2] = codz;
             double tmc_local = grid.AccessToTmc(coords);
             long double exp_eqn_rhs = pow( pow(lambda, m) * K1 * tmc_local + pow(L_initial, m), (double)n/m) - pow(L0, n); //pow(lambda*(1+K1*pow(tmc_local, n1)), n) - pow(L0, n);
             long double temperature = grid.AccessToTmp(coords);
             exp_eqn_rhs += K_*t_inc*exp(-Q/R/temperature);
             grid.AccessToTmc(coords) = (pow(exp_eqn_rhs + pow(L0, n), (double)m/n) - pow(L_initial, m)) / pow(lambda, m) / K1;//pow(1.0/K1/lambda*pow(exp_eqn_rhs+pow(L0, n),1.0/n)-1.0/K1, 1.0/n1);  
           }
   }
}

template <int dim> void calculateGrainSizeDist(MMSP::grid<dim, unsigned long>& grid, int grains_along_line[]){
	int lowbound = x0(grid, 1), upbound = x1(grid, 1);
	for(int codx = x0(grid, 0); codx < x1(grid, 0); codx++) { // x direction is where temperature varies
		grains_along_line[codx] = 0;
		int cody = lowbound;
		long long prevgid = -1;
		while(cody < upbound) {
			if(prevgid == -1) {
				grains_along_line[codx]++;
				vector<int> coords(dim,0);
				coords[0] = codx, coords[1] = cody;
				prevgid = grid(coords);
			}
			else {
				vector<int> coords(dim,0);
				coords[0] = codx, coords[1] = cody, coords[2] = 0;
				if(prevgid != grid(coords)) {
					grains_along_line[codx]++;
					prevgid = grid(coords);
				}
			}
			cody++;
		}
		if(x1(grid, 1) < g1(grid, 1)) { // if the current partition is not the edge partition.
			vector<int> coords(dim,0);
			coords[0] = codx, coords[1] = upbound, coords[2] = 0;
			if(grid(coords) == prevgid && grains_along_line[codx] != 0) {
				grains_along_line[codx]--;
			}
		}
	}
}

template <int dim> unsigned long update(MMSP::grid<dim, unsigned long>& grid, int steps, long int steps_finished, int nthreads, int step_to_nonuniform, long double &physical_time, double velInverse, double maxTemp, double minTemp, double plat, double range) {
	unsigned long start = rdtsc();

	#if (!defined MPI_VERSION) && ((defined CCNI) || (defined BGQ))
	std::cerr<<"Error: MPI is required for CCNI."<<std::endl;
	exit(1);
	#endif
	int rank=0;
	unsigned int np=0;
//	#ifdef MPI_VERSION
	rank=MPI::COMM_WORLD.Get_rank();
	np=MPI::COMM_WORLD.Get_size();
	MPI::COMM_WORLD.Barrier();
//	#endif

	unsigned long update_timer = 0;

	pthread_t* p_threads = new pthread_t[nthreads];
	flip_index<dim>* mat_para = new flip_index<dim> [nthreads];
	pthread_attr_t attr;
	pthread_attr_init (&attr);

	#ifndef SILENT
	static int iterations = 1;
	#endif
	ghostswap(grid); 
/*
  int edge = 1000;
  int number_of_fields = static_cast<int>(float(edge*edge)/(M_PI*10.*10.)); // average grain is a disk of radius 10
  #ifdef MPI_VERSION
	while (number_of_fields % np) --number_of_fields;
  #endif
*/
/*
  EulerAngles *grain_orientations = new EulerAngles[200];
  if(rank==0){
    ReadData(grain_orientations);
  }
	MPI::COMM_WORLD.Barrier();
  MPI::COMM_WORLD.Bcast(grain_orientations, 200*3, MPI_DOUBLE, 0);
*/
/*-----------------------------------------------*/
/*---------------generate cells------------------*/
/*-----------------------------------------------*/
  int dimension_length=0, number_of_lattice_cells=1;
  int lattice_cells_each_dimension[dim];
  for(int i=0; i<dim; i++){
    dimension_length = x1(grid, i)-x0(grid, i);
//    dimension_length = x1(grid, i)-1-x0(grid, i);
    if(x0(grid, 0)%2==0)
      lattice_cells_each_dimension[i] = dimension_length/2+1;
    else
      lattice_cells_each_dimension[i] = 1+LargeNearestInteger(dimension_length,2);
    number_of_lattice_cells *= lattice_cells_each_dimension[i];
  }
  #ifndef SILENT
  if(rank==0){ 
    std::cout<<"lattice_cells_each_dimension are:  ";
    for(int i=0; i<dim; i++)
      std::cout<<lattice_cells_each_dimension[i]<<"  ";
    std::cout<<"\n";
  }
  #endif
//----------assign cells for each pthreads
  int num_of_cells_in_thread = number_of_lattice_cells/nthreads;
  //check if num of the pthread is too large, if so, reduce it.                                                                                                                                         
  if (num_of_cells_in_thread<1) {
    std::cerr<<"ERROR: number of pthread is too large, please reduce it to a value <= "<<number_of_lattice_cells<<std::endl;
    exit(0);
  }
//  int model_dimension=(g1(grid, 0)-g0(grid, 0)+1);
//  double *temperature_along_x = new double[model_dimension];

//  if(rank==0){
//    ReadTemperature(temperature_along_x, model_dimension);
//  }
//	MPI::COMM_WORLD.Barrier();
//  MPI::COMM_WORLD.Bcast(temperature_along_x, model_dimension, MPI_DOUBLE, 0);

	vector<int> x (dim,0);
	vector<int> x_prim (dim,0);
  int coordinates_of_cell[dim];
  int initial_coordinates[dim];
  int **cell_coord = new int*[nthreads];//record the start coordinates of each pthread domain.
  for(int i=0; i<nthreads; i++){
    cell_coord[i] = new int[dim];
    for(int j=0; j<dim; j++){
      cell_coord[i][j]=0;
    }
  }

  int **num_of_grids_to_flip = new int*[nthreads];
  for(int i=0; i<nthreads; i++){
    num_of_grids_to_flip[i] = new int[( static_cast<int>(pow(2,dim)) )];
    for(int j=0; j<pow(2,dim); j++){
      num_of_grids_to_flip[i][j]=0;
    }
  }

  for(int k=0; k<dim; k++) 
    initial_coordinates[k] = x0(grid, k);
  for(int i=0; i<dim; i++){
    if(x0(grid, i)%2!=0) 
      initial_coordinates[i]--;
  }

  for (int i=0; i<nthreads; i++) {
        int cell_numbering = num_of_cells_in_thread*i; //0-indexed, celling_numbering is the start cell numbering
        if(dim==2){
          cell_coord[i][dim-1]=cell_numbering%lattice_cells_each_dimension[dim-1];//0-indexed
          cell_coord[i][0]=(cell_numbering/lattice_cells_each_dimension[dim-1]);
	        if(cell_coord[i][0]>=lattice_cells_each_dimension[0]){
	          std::cerr<<"the cell coordinates is wrong!"<<std::endl;
	          exit(1);
	        }
        }else if(dim==3){
          cell_coord[i][dim-1]=cell_numbering%lattice_cells_each_dimension[dim-1];//0-indexed
          cell_coord[i][1]=(cell_numbering/lattice_cells_each_dimension[dim-1])%lattice_cells_each_dimension[1];
          cell_coord[i][0]=(cell_numbering/lattice_cells_each_dimension[dim-1])/lattice_cells_each_dimension[1];
	        if(cell_coord[i][0]>=lattice_cells_each_dimension[0]){
	          std::cerr<<"the cell coordinates is wrong!"<<std::endl;
	          exit(1);
	        }
        }

    mat_para[i].grid = &grid;
    if(i==(nthreads-1)) 
      mat_para[i].num_of_cells_in_thread = number_of_lattice_cells - num_of_cells_in_thread*(nthreads-1);
    else 
      mat_para[i].num_of_cells_in_thread = num_of_cells_in_thread;

    #ifndef SILENT
    if(rank==0) std::cout<<"num_of_cells_in_thread is "<<mat_para[i].num_of_cells_in_thread<<" in thread "<<i<<"\n";
    #endif

    for(int k=0; k<dim; k++) 
      mat_para[i].lattice_cells_each_dimension[k]=lattice_cells_each_dimension[k];

//    mat_para[i].temperature_along_x = temperature_along_x;
//    mat_para[i].grain_orientations = grain_orientations;

    for(int j=0; j<mat_para[i].num_of_cells_in_thread; j++){
      int start_cell_numbering = num_of_cells_in_thread*i;
      if(dim==2){
        coordinates_of_cell[dim-1]=(start_cell_numbering+j)%lattice_cells_each_dimension[dim-1];//0-indexed
        coordinates_of_cell[0]=(start_cell_numbering+j)/lattice_cells_each_dimension[dim-1];
      }else if(dim==3){
        coordinates_of_cell[dim-1]=(start_cell_numbering+j)%lattice_cells_each_dimension[dim-1];//0-indexed
        coordinates_of_cell[1]=((start_cell_numbering+j)/lattice_cells_each_dimension[dim-1])%lattice_cells_each_dimension[1];
        coordinates_of_cell[0]=((start_cell_numbering+j)/lattice_cells_each_dimension[dim-1])/lattice_cells_each_dimension[1];
      }
      for(int ii=0; ii<dim; ii++){
        x[ii]=initial_coordinates[ii]+2*coordinates_of_cell[ii];
      }

      if(dim==2){
        x_prim = x;
        if(!OutsideDomainCheck<dim>(grid, &x_prim)) num_of_grids_to_flip[i][0]+=1;

        x_prim = x;
        x_prim[1]=x[1]+1; //0,1 
        if(!OutsideDomainCheck<dim>(grid, &x_prim)) num_of_grids_to_flip[i][1]+=1;

        x_prim = x;
        x_prim[0]=x[0]+1; //1,0
        if(!OutsideDomainCheck<dim>(grid, &x_prim)) num_of_grids_to_flip[i][2]+=1;

        x_prim = x;
        x_prim[0]=x[0]+1;
        x_prim[1]=x[1]+1; //1,1 
        if(!OutsideDomainCheck<dim>(grid, &x_prim)) num_of_grids_to_flip[i][3]+=1;
      }else if(dim==3){
        x_prim = x;//0,0,0
        if(!OutsideDomainCheck<dim>(grid, &x_prim)) num_of_grids_to_flip[i][0]+=1;

        x_prim = x;
        x_prim[2]=x[2]+1; //0,0,1 
        if(!OutsideDomainCheck<dim>(grid, &x_prim)) num_of_grids_to_flip[i][1]+=1;

        x_prim = x;
        x_prim[1]=x[1]+1; //0,1,0
        if(!OutsideDomainCheck<dim>(grid, &x_prim)) num_of_grids_to_flip[i][2]+=1;

        x_prim = x;
        x_prim[2]=x[2]+1;
        x_prim[1]=x[1]+1; //0,1,1 
        if(!OutsideDomainCheck<dim>(grid, &x_prim)) num_of_grids_to_flip[i][3]+=1;

        x_prim = x;
        x_prim[0]=x[0]+1; //1,0,0 
        if(!OutsideDomainCheck<dim>(grid, &x_prim)) num_of_grids_to_flip[i][4]+=1;

        x_prim = x;
        x_prim[2]=x[2]+1;
        x_prim[0]=x[0]+1; //1,0,1 
        if(!OutsideDomainCheck<dim>(grid, &x_prim)) num_of_grids_to_flip[i][5]+=1;

        x_prim = x;
        x_prim[1]=x[1]+1;
        x_prim[0]=x[0]+1; //1,1,0 
        if(!OutsideDomainCheck<dim>(grid, &x_prim)) num_of_grids_to_flip[i][6]+=1;

        x_prim = x;
        x_prim[2]=x[2]+1;
        x_prim[1]=x[1]+1;
        x_prim[0]=x[0]+1; //1,1,1 
        if(!OutsideDomainCheck<dim>(grid, &x_prim)) num_of_grids_to_flip[i][7]+=1;
      }
    }// for int j 
  }//for int i

	for (int step=0; step<steps; step++){
		double position[241] = {0.0};
		double pointtemp[241] = {0.0};

		if (steps_finished + step == 0) {
			if(rank == 0) {
		    	char orgpath[256];
		    	char *path = getcwd(orgpath, 256);

			    int rc = chdir("/home/smartcoder/Documents/Developer/InverseGG/thermal/ColumnarGrowthTempInverse/");
			    if (rc < 0) {
			        std::cerr << "wrong working directory" << std::endl;
			    }

				std::string cmd = "./driver";
				cmd += " " + std::to_string(grad_pos_start * domainLen / dim_x) + " " + std::to_string(grad_pos_end * domainLen / dim_x);
			    char buffer[128];
			    std::string result = "";
			    std::cout << "cmd is  " << cmd << std::endl;
			    FILE* pipe = popen(cmd.c_str(), "r");
			    if (!pipe) throw std::runtime_error("popen() failed!");
			    try {
			        while (!feof(pipe)) {
			            if (fgets(buffer, 128, pipe) != NULL) {
			                result += buffer;
			                //std::cout << "buffer is " << buffer << std::endl;
			            }
			        }
			    } catch (...) {
			        pclose(pipe);
			        throw;
			    }
			    pclose(pipe);

			    rc = chdir(orgpath);
			    if (rc < 0) {
			        std::cerr << "switch back working directory failed" << std::endl;
			    }

			    //std::cout << "result is \n" << result << std::endl;
			    std::stringstream ss(result);
			    int index = 0; 
			    //std::cout << "ss is \n" << ss.str() << std::endl;
			    while(ss >> position[index] && ss >> pointtemp[index++]) {}
			    //for(int tt = 0; tt < 241; tt++) std::cout << std::setw(10) << pointtemp[tt];
			    //std::cout << "\n\n" << std::endl;
			    for(int nd = 0; nd < 241; nd++) {
			    	position_stack[nd] = position[nd];
			    	pointtemp_stack[nd] = pointtemp[nd];
			    }
			}
			MPI_Bcast(position, 241, MPI_DOUBLE, 0, MPI_COMM_WORLD);
			MPI_Bcast(pointtemp, 241, MPI_DOUBLE, 0, MPI_COMM_WORLD);
			UpdateLocalTmp(grid, position, pointtemp, maxTemp, minTemp);
		}


	    double tmc_at_PdenominatorMax = 0.0;
	    double tmp_at_PdenominatorMax = 0.0;
	    double Pdenominator_max_partition = PdenominatorMax(grid, tmc_at_PdenominatorMax, tmp_at_PdenominatorMax); 
	    MPI::COMM_WORLD.Barrier();
	    double Pdenominator_max_global = 0.0;

	    MPI::COMM_WORLD.Allreduce(&Pdenominator_max_partition, &Pdenominator_max_global, 1, MPI_DOUBLE, MPI_MAX);
	    MPI::COMM_WORLD.Barrier();

	    double tmc_at_PdenominatorMax_global = 0.0;
	    double tmp_at_PdenominatorMax_global = 0.0;
	    if(Pdenominator_max_partition != Pdenominator_max_global){
	      tmc_at_PdenominatorMax = 0.0;
	      tmp_at_PdenominatorMax = 0.0;
	    }
	    MPI::COMM_WORLD.Allreduce(&tmc_at_PdenominatorMax, &tmc_at_PdenominatorMax_global, 1, MPI_DOUBLE, MPI_MAX);
	    MPI::COMM_WORLD.Barrier();
	    MPI::COMM_WORLD.Allreduce(&tmp_at_PdenominatorMax, &tmp_at_PdenominatorMax_global, 1, MPI_DOUBLE, MPI_MAX);
	    MPI::COMM_WORLD.Barrier();

	    double t_inc = (pow(pow(lambda, m) * K1 * (tmc_at_PdenominatorMax_global + 1) + pow(L_initial, m), (double)n/m) - pow(pow(lambda, m) * K1 * tmc_at_PdenominatorMax_global + pow(L_initial, m), (double)n/m)) / K_/exp(-Q/R/tmp_at_PdenominatorMax_global); //(pow(lambda*(1+K1*pow(tmc_at_PdenominatorMax_global+1, n1)), n) - 
	      // pow(lambda*(1+K1*pow(tmc_at_PdenominatorMax_global, n1)), n) )/K_/exp(-Q/R/tmp_at_PdenominatorMax_global);

		
	    int num_of_sublattices = 0;
	    if(dim==2) num_of_sublattices = 4; 
	    else if(dim==3) num_of_sublattices = 8;
			for (int sublattice=0; sublattice < num_of_sublattices; sublattice++) {
				for (int i=0; i!= nthreads ; i++) {
					mat_para[i].sublattice=sublattice;
					mat_para[i].num_of_points_to_flip=num_of_grids_to_flip[i][sublattice];
	        mat_para[i].Pdenominator = Pdenominator_max_global;
	        for(int k=0; k<dim; k++) mat_para[i].cell_coord[k]=cell_coord[i][k];
					pthread_create(&p_threads[i], &attr, flip_index_helper<dim>, (void*) &mat_para[i] );
				}//loop over threads

				for (int ii=0; ii!= nthreads ; ii++)
					pthread_join(p_threads[ii], NULL);

				#ifdef MPI_VERSION
				MPI::COMM_WORLD.Barrier();
				#endif

				ghostswap(grid, sublattice); // once looped over a "color", ghostswap.
	//			ghostswap(grid); // once looped over a "color", ghostswap.
	            #ifdef MPI_VERSION
				MPI::COMM_WORLD.Barrier();
	                #endif
			}//loop over color

		int grains_along_line[dim_x] = {0};
		int grains_along_line_global[dim_x] = {0};
		calculateGrainSizeDist(grid, grains_along_line);

		/*
		for(int i = 0; i < 4; i++) {
			MPI::COMM_WORLD.Barrier();
			if(rank == i) {
				for(int i = 0; i < dim_x; i++) std::cout << 	grains_along_line[i] << " ";
				std::cout << std::endl;
			}
			MPI::COMM_WORLD.Barrier();
		}
		*/

	    MPI_Reduce(grains_along_line, grains_along_line_global, dim_x, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

		//if((steps_finished + step) % 100 == 0 && rank==0) {
	    if(rank==0) { // do not fix the update time interval
			// first dump all the raw data on disk
			/*
			std::ofstream ofs;
			ofs.open("size_history.txt", std::ofstream::out | std::ofstream::app);	
			for(int i = 0; i < dim_x; i++) {
				ofs << grains_along_line_global[i] << " ";
			}
			ofs << "\n";
			ofs.close();
			*/
			// smooth the gradient in the data to detect start and end 
			int sum = 0;
			double mostdense = 0.0, mostsparse = 1.0 * dim_x; 
			double grains_along_line_global_smoothed[dim_x];
			bool shouldUpdate = false;
			if(mid_check <= grad_pos_start + 0.25 * (grad_pos_end - grad_pos_start)) mid_check = grad_pos_end;
			std::cout << grains_along_line_global[grad_pos_start] << "  " << grains_along_line_global[mid_check] << std::endl;
			
			if(abs(grains_along_line_global[mid_check] - grains_along_line_global[grad_pos_start]) < 0.1*grains_along_line_global[grad_pos_start]) {
				//int delta = mid_check - grad_pos_start;
				int delta = (mid_check - grad_pos_start) / 2; // delta is a very important parameter to make columnar!
				grad_pos_start += delta;
				grad_pos_end += delta;
				shouldUpdate = true;
			}
			mid_check--;
			
			
			/*
			int loc_max_deriv = 0; 
			double max_deriv = 0.0;
			for(int i = 0; i < dim_x; i++) {
				if(i!= 0 && grains_along_line_global[i] - grains_along_line_global[i - 1] > max_deriv) {
					max_deriv = grains_along_line_global[i] - grains_along_line_global[i - 1] ;
					loc_max_deriv = i;
				}
				mostsparse = min(mostsparse, (double)grains_along_line_global[i]);
				mostdense = max(mostdense, (double)grains_along_line_global[i]);
			}
			*/
			/*
			//grad_pos_start = loc_max_deriv;
			//grad_pos_end = grad_pos_start + 10;
			if((steps_finished + step) % 100 == 0) {
				grad_pos_start++;
				grad_pos_end++;
			}
			
			//std::cout << mostsparse << " " << mostdense << " " << grad_pos_start << " " << grad_pos_end << std::endl;
			ofs.open("size_history_smooth.txt", std::ofstream::out | std::ofstream::app);	
			for(int i = 0; i < dim_x; i++) {
				ofs << grains_along_line_global_smoothed[i] << " ";
			}
			ofs << "\n\n";
			ofs.close();
			*/
			if(shouldUpdate) {

		    	char orgpath[256];
		    	char *path = getcwd(orgpath, 256);

			    int rc = chdir("/home/smartcoder/Documents/Developer/InverseGG/thermal/ColumnarGrowthTempInverse/");
			    if (rc < 0) {
			        std::cerr << "wrong working directory" << std::endl;
			    }

				std::string cmd = "./driver";
				cmd += " " + std::to_string(grad_pos_start * domainLen / dim_x) + " " + std::to_string(grad_pos_end * domainLen / dim_x);
			    char buffer[128];
			    std::string result = "";
			    std::cout << "cmd is  " << cmd << std::endl;
			    FILE* pipe = popen(cmd.c_str(), "r");
			    if (!pipe) throw std::runtime_error("popen() failed!");
			    try {
			        while (!feof(pipe)) {
			            if (fgets(buffer, 128, pipe) != NULL) {
			                result += buffer;
			                //std::cout << "buffer is " << buffer << std::endl;
			            }
			        }
			    } catch (...) {
			        pclose(pipe);
			        throw;
			    }
			    pclose(pipe);

			    rc = chdir(orgpath);
			    if (rc < 0) {
			        std::cerr << "switch back working directory failed" << std::endl;
			    }

			    //std::cout << "result is \n" << result << std::endl;
			    std::stringstream ss(result);
			    int index = 0; 
			    //std::cout << "ss is \n" << ss.str() << std::endl;
			    while(ss >> position[index] && ss >> pointtemp[index++]) {}
			    //for(int tt = 0; tt < 241; tt++) std::cout << std::setw(10) << pointtemp[tt];
			    //std::cout << "\n\n" << std::endl;
			    for(int nd = 0; nd < 241; nd++) {
			    	position_stack[nd] = position[nd];
			    	pointtemp_stack[nd] = pointtemp[nd];
			    }
			}
			else {
			    for(int nd = 0; nd < 241; nd++) {
			    	position[nd] = position_stack[nd];
			    	pointtemp[nd] = pointtemp_stack[nd];
			    }
			}
		}
		MPI_Bcast(position, 241, MPI_DOUBLE, 0, MPI_COMM_WORLD);
		MPI_Bcast(pointtemp, 241, MPI_DOUBLE, 0, MPI_COMM_WORLD);

		physical_time += t_inc;
		UpdateLocalTmc(grid, t_inc);
		//if((steps_finished + step) % 100 == 0) {
		UpdateLocalTmp(grid, position, pointtemp, maxTemp, minTemp);
		//}

		//if(rank == 0) std::cout << "grad_pos_start:" << grad_pos_start << "   grad_pos_end:" << grad_pos_end << std::endl;

	    /*
	    if(rank == 0) {
	    	for(int i = 0; i < dim_x; i++) {
	    		std::cout << "pos: " << i << "  #grains: " << grains_along_line_global[i] << std::endl;
	    	}
	    }
	    */

		update_timer += rdtsc()-start;
	}//loop over step

	#ifndef SILENT
	++iterations;
	#endif

  for(int i=0; i<nthreads; i++){
    delete [] num_of_grids_to_flip[i];
    num_of_grids_to_flip[i]=NULL;
    delete [] cell_coord[i];
    cell_coord[i]=NULL;
  }
  delete num_of_grids_to_flip; 
  num_of_grids_to_flip=NULL; 
  delete cell_coord;
  cell_coord=NULL;
	delete [] p_threads;
	p_threads=NULL;
	delete [] mat_para;
	mat_para=NULL;

	unsigned long total_update_time=update_timer;
	#ifdef MPI_VERSION
	MPI::COMM_WORLD.Allreduce(&update_timer, &total_update_time, 1, MPI_UNSIGNED_LONG, MPI_SUM);
  MPI::COMM_WORLD.Barrier();
	#endif
	return total_update_time/np; // average update time
}

}


#endif

#include"MMSP.main.hpp"

