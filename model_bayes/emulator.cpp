/* I just realized there is a fairly large flaw in my output
	matrix in the GPS. I account for multiple obervables in 
	the mean and std output, but not the generated functions.
	Update in future. Easy fix.

	***********************************************************/


#include<iostream>
#include<iomanip>
#include<cmath>
#include<cstdlib>
#include<random>
#include<chrono>
#include<fstream>
#include<string>
#include "time.h"
#include "armadillo"
#include "gaussianprocess.cpp"

std::ofstream ofile;
std::ifstream ifile;

double function(arma::vec param){
	double value = param(0) + param(0)*sin(1.5*param(0));
	return value;
}
bool identical(arma::vec test){
	int i, j, length = test.n_elem;
	bool value = false;
	for(i=0;i<length;i++){
		for(j=i+1;j<length;j++){
			if(test(i)==test(j)){
				value = true;
				break;
			}
		}
	}
	return value;
}
arma::vec hyperp_optimize_function(
	arma::mat Y_mat, arma::mat X_mat, 
	int resolution, double precision, 
	arma::mat kernal_func(arma::mat, arma::mat, arma::vec), 
	arma::vec hyperp_guess_vec, arma::mat& hyperp_range_mat);

int main(int argc, char* argv[]){
	//Variables for use throughout the program
	int i,
		train, test, 
		param, observables, hyperp,
		samples;
	unsigned seed;
	double 	x, epsilon;
	double dx, x_init, x_final;
	double sigma_n, sigma_f, l;

	//For writing to file
	std::string infilename, outfilename;

	//Test to ensure the proper input. Terminate program if command line inputs are not given
	if(argc<3){
		std::cout << "Improper entry. Please also enter 'infilename outfilename' on same line." << std::endl;
		exit(1);
	}
	else{
		infilename = argv[1];
		outfilename = argv[2];
	}

	/*SYSTEM and OUTPUT--
	***********************************************************/

	train = 26;
	test = 100;

	samples = 3;
	param = 1;
	observables = 1;

	epsilon = 1e-8;

	seed = std::chrono::system_clock::now().time_since_epoch().count();

	/*HYPERPARAMETERS--
	***********************************************************/

	hyperp = 3;
	sigma_n = 0.0001;
	sigma_f = 0.003;
	l = 0.25;
		

	//random number generator
	std::default_random_engine generator(seed);
	std::normal_distribution<double> dist(0,1.0);

	//armadillo matrices and vectors for computational use
	arma::mat xvec_train_mat = arma::zeros<arma::mat>(train,param),
		  yvec_train_mat = arma::zeros<arma::mat>(train,observables),
		  xvec_test_mat = arma::zeros<arma::mat>(test,param),

		  output_mat = arma::zeros<arma::mat>(test,param + 3 + samples);

	arma::vec hyperp_vec = arma::zeros<arma::vec> (hyperp),
		  param_x_vec = arma::zeros<arma::vec> (param),
	  dely_vec = arma::zeros<arma::vec> (train),
	  BF_vec = arma::zeros<arma::vec> (train),
	  error_vec = arma::zeros<arma::vec> (train);

	//hyperparamter vector for use in 'function'
	hyperp_vec(0) = sigma_f;
	hyperp_vec(1) = l;
	hyperp_vec(2) = sigma_n;

	//Generate Functions	
	ifile.open(infilename);
	for(i=0;i<train;i++){
	  ifile >> dely_vec(i);
	  ifile >> BF_vec(i);
	  ifile >> error_vec(i);
	  xvec_train_mat(i,0) = dely_vec(i);
	  yvec_train_mat(i,0) = BF_vec(i);
	}
	ifile.close();
	x_init = dely_vec(0);
	x_final = dely_vec(train-1);
	dx = (x_final - x_init)/(double) test;
	for(i=0;i<test;i++){
	  xvec_test_mat(i,0) = x_init + ((double) i)*dx;
	}

	//Execute posterior function generation
	output_mat = gaussian_process_solver_basic(kernal_square_exp_noise_function, xvec_train_mat, yvec_train_mat, xvec_test_mat, samples, hyperp_vec, epsilon);

	//Write the output file
	write_output(output_mat, test, param, observables, samples, outfilename);

	//Write the trainingset to file
	write_trainset(xvec_train_mat, yvec_train_mat, "trainset.dat");

	return 0;
}

arma::vec hyperp_optimize_function(
	arma::mat Y_mat, arma::mat X_mat, 
	int resolution, double precision, 
	arma::mat kernal_func(arma::mat, arma::mat, arma::vec), 
	arma::vec hyperp_guess_vec, arma::mat& hyperp_range_mat){
	
	int i,
		zeros, selection,
		hyperp = hyperp_guess_vec.n_elem,
		count = 0;

	double theta_i, theta_f, dtheta, scale;

	arma::vec ll_der_vec = arma::zeros<arma::vec>(resolution),
				zeros_vec,
				hyperp_values_vec,
				ll_vec,
				hyperp_vec = hyperp_guess_vec,
				temp_vec;

	for(i=0;i<hyperp;i++){
			hyperp_vec(i) = hyperp_guess_vec(i);
	}

	for(i=0;i<hyperp;i++){
		selection = i;
		zeros=0;
		std::cout << "hyperparameter" << selection << ":" << std::endl;
		count=0;
		
	hyperp_vec.print();
		while(zeros==0&count<5){
			ll_der_vec =  log_likelihood_derivative(Y_mat, X_mat, resolution, kernal_func, hyperp_vec, hyperp_range_mat, selection, precision);
			zeros_vec = find_zeros(ll_der_vec);
			zeros = zeros_vec.n_elem-1;
			if(zeros==1){
				theta_i = hyperp_range_mat(0,selection);
				theta_f = hyperp_range_mat(1,selection);
				dtheta = (theta_f - theta_i)/(double) (resolution-1);
				scale = (theta_f - theta_i)/theta_f;
				count = 0;
				while(scale>0.01){
					theta_i = hyperp_range_mat(0,selection);
					theta_f = hyperp_range_mat(1,selection);
					dtheta = (theta_f - theta_i)/(double) (resolution-1);

					std::cout << "hyperp" << selection << ":" << std::endl;
					std::cout << std::setw(15) << "zeros: " << zeros_vec(0) << std::setw(15) << "hyperp:" << hyperp_values_vec(0) << std::endl;					
					std::cout << std::setw(15) << "searching in " << theta_i << " to " << theta_f << std::endl;
					
					ll_der_vec =  log_likelihood_derivative(Y_mat, X_mat, resolution, kernal_func, hyperp_vec, hyperp_range_mat, selection, precision);
					zeros_vec = find_zeros(ll_der_vec);
					zeros = zeros_vec.n_elem-1;

					theta_i += dtheta*(double) (zeros_vec(0) - 1);
					theta_f = theta_i + 2.0*dtheta;

					hyperp_range_mat(0,selection) = theta_i;
					hyperp_range_mat(1,selection) = theta_f;
					scale = (theta_f - theta_i)/theta_f;

					std::cout << std::setw(15) << "new zero at " << theta_i + dtheta;
				}
				hyperp_vec(selection) = theta_i + dtheta;
			}
			else if(zeros>1){
				std::cout << "Multiple zeros found for hyperparamter" << selection << ", skipping to next." << std::endl;
			}
			else{
				hyperp_range_mat(1,selection) *= 2.0;
				count++;
				if(count==5){
					std::cout << "Search range reached, skipping to next hyperparameter." << std::endl;
				}
			}
		}
	}
	return hyperp_vec;
}







