/* COMMENTS SECTION:


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

int main(int argc, char* argv[]){
	//Variables for use throughout the program
  int i,j,
    train, test, 
    param, observables, hyperp,
    samples;
  unsigned seed;
  double 	x, epsilon;
  double dx, x_init, x_final;
  double sigma_n, sigma_f, l;

  /*
  sigma_n = 0.01;
  sigma_f = 0.1;
  l = 0.1;
  */

  //For writing to file
  std::string param_filename, obs_filename, outfilename, trainfilename;

  //Test to ensure the proper input. Terminate program if command line inputs are not given
  if(argc<6){
    std::cout << "Improper entry. Please also enter 'param_filename obs_filename sigma_n sigma_f l' on same line." << std::endl;
    exit(1);
  }
  else{
    param_filename = argv[1];
    obs_filename = argv[2];
    sigma_n = atof(argv[3]);
    sigma_f = atof(argv[4]);
    l = atof(argv[5]);
  }
  
  /*SYSTEM and OUTPUT--
***********************************************************/
  
  train = 1000;
  test = 719;
  
  samples = 1;
  param = 4;
  observables = 12;

  epsilon = 1e-8;
  
  seed = std::chrono::system_clock::now().time_since_epoch().count();
  
  outfilename = "testset.dat";
  trainfilename = "trainset.dat";
  
  /*HYPERPARAMETERS--
***********************************************************/
  
  hyperp = 3;
  
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
    BF_vec = arma::zeros<arma::vec> (train),
    error_vec = arma::zeros<arma::vec> (train);
  
  //hyperparamter vector for use in 'function'
  hyperp_vec(0) = sigma_f;
  hyperp_vec(1) = l;
  hyperp_vec(2) = sigma_n;
  
  //Generate Functions	
  
  ifile.open(param_filename);
  for(i=0;i<train;i++){
    for(j=0;j<param;j++){
      ifile >> xvec_train_mat(i,j);
    }
  }
  ifile.close();
  ifile.open(obs_filename);
  for(i=0;i<train;i++){
    for(j=0;j<observables;j++){
      ifile >> yvec_train_mat(i,j);
    }
  }
  ifile.close();
  observables=1;
  yvec_train_mat.resize(train,1);
  /*
  train = 50;
  test = 30;
  param = 1;
  observables=1;
  xvec_train_mat = arma::zeros<arma::mat>(train,1);
  yvec_train_mat = arma::zeros<arma::mat>(train,1);
  for(int i=0;i<train;i++){
    xvec_train_mat(i,0) = (double) (i)/5;
    yvec_train_mat(i,0) = sin((double) (i)/5);
  }
  */

  int lhp_samples = test;
  arma::mat hypercube = arma::zeros<arma::mat>(lhp_samples,param);
  arma::vec hyperlist = arma::linspace<arma::vec>(0,lhp_samples-1,lhp_samples);
  for(int i=0;i<param;i++){
    hyperlist=shuffle(hyperlist);
    hypercube.col(i) = hyperlist;
  }
  
  /*Now, construct full numerical lhp sampling using the input from file
    and the previous generic sampling matrix
  */
  float init,final;
  
  for(int i=0;i<param;i++){
    init = 0.01;
    final = 2.0;
    dx = (final-init)/(lhp_samples-1);
    hyperlist = hypercube.col(i);
    hyperlist = init + dx*hyperlist;
    hypercube.col(i) = hyperlist;
  }
  
  for(i=0;i<test;i++){
    for(j=0;j<param;j++){
      xvec_test_mat(i,j) = hypercube(i,j);
    }
  }

  //Execute posterior function generation
  //	output_mat = gaussian_process_solver_basic(kernal_square_exp_noise_function, xvec_train_mat, yvec_train_mat, xvec_test_mat, samples, hyperp_vec, epsilon);
  output_mat = gaussian_process_solver_basic(kernel_square_exp_noise_function, xvec_train_mat, yvec_train_mat, xvec_test_mat, samples, hyperp_vec, epsilon);
  //Write the output file
  write_output(output_mat, test, param, observables, samples, outfilename);
  //Write the trainingset to file
  write_trainset(xvec_train_mat, yvec_train_mat, trainfilename);
  //Log Likelihood
  //  std::cout << "log likelihood_function:\n" << log_likelihood_function(yvec_train_mat, xvec_train_mat, kernel_square_exp_function, hyperp_vec) << std::endl;
  /*
  arma::vec mean;
  arma::mat variance;
  double log_likelihood;

  arma::vec y_vec = yvec_train_mat.col(0);
  xvec_train_mat = xvec_train_mat.t();
  xvec_test_mat = xvec_test_mat.t();
  gaussian_process(xvec_train_mat, y_vec, kernel_square_exp_noise_function, hyperp_vec, sigma_n, xvec_test_mat, mean, variance, log_likelihood);
  std::cout << "LL:\n" << log_likelihood << std::endl;
  */

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







