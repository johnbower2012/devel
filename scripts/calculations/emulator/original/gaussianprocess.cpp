#include "gaussianprocess.hpp"

/**KERNEL FUCNTIONS
**********************************************/
//calculate the kernal of vector sets A and B, using hyperparameters in hyperp, given each has dimensionality param,
//where A and B are the form C = [X1,X2,...] where each Xi.t = [param_i_1, param_i_2, ...].
arma::mat kernel_square_exp_noise_function(arma::mat A, arma::mat B, arma::vec hyperp_vec){
	double xi, xj, sum, diff,
			theta1, theta2, theta3;
	int i, j, k,
		ai = A.n_rows, bi = B.n_rows,
		param = A.n_cols;

	arma::mat kernel_matrix = arma::zeros<arma::mat> (ai, bi);

	theta1 = hyperp_vec(0)*hyperp_vec(0);
	theta2 = 1.0/2.0/hyperp_vec(1)/hyperp_vec(1);
	theta3 = hyperp_vec(2)*hyperp_vec(2);

	if(ai>bi){
		for(i=0;i<ai;i++){
			for(j=0;j<bi;j++){
				sum = 0.0;
				for(k=0;k<param;k++){
					xi = A(i,k);
					xj = B(j,k);
					diff = xi - xj;
					sum += diff*diff;
				}
				kernel_matrix(i,j) = theta1*exp(-sum*theta2);
			}
		}
		for(i=0;i<bi;i++){
			kernel_matrix(i,i) += theta3;
		}
	}
	else{
		for(i=0;i<ai;i++){
			for(j=0;j<bi;j++){
				sum = 0.0;
				for(k=0;k<param;k++){
					xi = A(i,k);
					xj = B(j,k);
					diff = xi - xj;
					sum += diff*diff;
				}
				kernel_matrix(i,j) = theta1*exp(-sum*theta2);
			}
			kernel_matrix(i,i) += theta3;
		}
	}

	return kernel_matrix;
}
arma::mat kernel_square_exp_function(arma::mat A, arma::mat B, arma::vec hyperp_vec){
  double xi, xj, sum, diff,
    theta1, theta2;
  int i, j, k,
    ai = A.n_rows, bi = B.n_rows,
    param = A.n_cols;

  arma::mat kernel_matrix = arma::zeros<arma::mat> (ai, bi);

  theta1 = hyperp_vec(0)*hyperp_vec(0);
  theta2 = 1.0/hyperp_vec(1)/hyperp_vec(1)/2.0;

  for(i=0;i<ai;i++){
    for(j=0;j<bi;j++){
      sum = 0.0;
      for(k=0;k<param;k++){
	xi = A(i,k);
	xj = B(j,k);
	diff = xi - xj;
	sum += diff*diff;
      }
      kernel_matrix(i,j) = theta1*exp(-sum*theta2);
    }
  }

  return kernel_matrix;
}

/**GAUSSIAN PROCESS FUNCTIONS
**********************************************/

arma::mat gaussian_process_solver_regression(
	arma::mat kernel_func(arma::mat, arma::mat, arma::vec), arma::mat X_mat, arma::mat Y_mat, arma::mat X_s_mat, 
	int samples, arma::vec hyperp_vec, double epsilon, 
	arma::mat regression_func(arma::mat, arma::mat), arma::mat beta)
	{
	//random number generator
	unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
	std::default_random_engine generator(seed);
	std::normal_distribution<double> dist(0,1.0);

	int i, j,
	  train = X_mat.n_rows, test = X_s_mat.n_rows,
	  param = X_mat.n_cols, observables = Y_mat.n_cols,
	  hyperp = hyperp_vec.n_elem;

	double noise = hyperp_vec(hyperp-1)*hyperp_vec(hyperp-1);

	//armadillo matrices and vectors for computational use
	arma::mat kernel_mat = arma::zeros<arma::mat>(train,train),
	  kernel_inv_mat = kernel_mat,
	  kernel_s_mat = arma::zeros<arma::mat>(train,test),
	  kernel_ss_mat = arma::zeros<arma::mat>(test,test),

	  mean_mat = arma::zeros<arma::mat>(test,observables),
	  var_mat = kernel_ss_mat,
	  L = var_mat,

	  H_mat = arma::zeros<arma::mat>(1,train),
	  H_s_mat = arma::zeros<arma::mat>(1,test),
	  R_mat = arma::zeros<arma::mat>(train,test),
	  beta_mat = arma::zeros<arma::mat>(train,observables),

	  temp_pt = arma::zeros<arma::mat>(1,test),
	  temp_pp = arma::zeros<arma::mat>(1,1),
				
	  post_func = arma::zeros<arma::mat>(test,samples),
	  random_sample = post_func,
	  Mean_mat = post_func,

	  I_train = arma::eye<arma::mat>(train,train),
	  I_test = arma::eye<arma::mat>(test,test),

	  output_mat = arma::zeros<arma::mat>(test, param + 2*observables + samples);

	//calculate kernel matrices
	kernel_mat = kernel_func(X_mat, X_mat, hyperp_vec) + I_train*noise;
	kernel_s_mat = kernel_func(X_mat, X_s_mat, hyperp_vec);
	kernel_ss_mat = kernel_func(X_s_mat, X_s_mat, hyperp_vec);

	//numerical stability factor in solving the Cholesky Decomposition
	kernel_mat += I_train*epsilon;
	kernel_ss_mat += I_test*epsilon;
	kernel_inv_mat = kernel_mat.i();

	//calculate regression related matrices R and beta
	H_mat = regression_func(X_mat, beta);
	H_s_mat = regression_func(X_s_mat, beta);
	R_mat = H_s_mat - H_mat*kernel_inv_mat*kernel_s_mat;

	temp_pp = H_mat*kernel_inv_mat*H_mat.t();
	beta_mat = temp_pp.i()*H_mat*kernel_inv_mat*Y_mat;

	//calculate MEAN of distribution
	mean_mat = kernel_s_mat.t()*kernel_mat.i()*Y_mat + R_mat.t()*beta_mat;

	//Calculate COVARIANCE of distribution
	var_mat = kernel_ss_mat - kernel_s_mat.t()*kernel_mat.i()*kernel_s_mat + R_mat.t()*temp_pp.i()*R_mat;	

	//Cholesky Decomposition, 'sqrt' of Variance matrix, here the lower triangular form
	L = arma::chol(var_mat,"lower");

	//generator random distribution
	//create matrix of "Mean" values for easy generation of output function values
	for(i=0;i<samples;i++){
		for(j=0;j<test;j++){
			random_sample(j,i) = dist(generator);
			Mean_mat(j,i) = mean_mat(j,0);
		}
	}

	//sample posterior functions
	post_func = Mean_mat + L*random_sample;

	//generate output matrix:
	//		param1, param2, ... , mean1, var1, mean2, var2, ... , post_func1, post_func2, ...
	for(i=0;i<test;i++){
		for(j=0;j<param;j++){
			output_mat(i,j) = X_s_mat(i,j);
		}
		for(j=0;j<observables;j++){
			output_mat(i,j*2+param) = mean_mat(i,j);
			output_mat(i,j*2+1+param) = var_mat(i,i);
		}
		for(j=0;j<samples;j++){
			output_mat(i,j+param+observables*2) = post_func(i,j);
		}
	}

	return output_mat;
}

arma::mat gaussian_process_solver_basic(arma::mat kernel_func(arma::mat, arma::mat, arma::vec), arma::mat X_mat, arma::mat Y_mat, arma::mat X_s_mat, int samples, arma::vec hyperp_vec, double epsilon){
	//random number generator
	unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
	std::default_random_engine generator(seed);
	std::normal_distribution<double> dist(0,1.0);

	int i, j,
		train = X_mat.n_rows, test = X_s_mat.n_rows,
	  param = X_mat.n_cols, observables = Y_mat.n_cols,
	  hyperp = hyperp_vec.n_elem;

	double noise = hyperp_vec(hyperp-1)*hyperp_vec(hyperp-1);

	//armadillo matrices and vectors for computational use
	arma::mat 	kernel_mat = arma::zeros<arma::mat>(train,train),
	  kernel_s_mat = arma::zeros<arma::mat>(train,test),
	  kernel_ss_mat = arma::zeros<arma::mat>(test,test),

	  mean_mat = arma::zeros<arma::mat>(test,observables),
	  var_mat = kernel_ss_mat,
	  L = var_mat,
				
	  post_func = arma::zeros<arma::mat>(test,samples),
	  random_sample = post_func,
	  Mean_mat = post_func,

	  I_train = arma::eye<arma::mat>(train,train),
	  I_test = arma::eye<arma::mat>(test,test),

	  output_mat = arma::zeros<arma::mat>(test, param + 2*observables + samples);

	arma::vec param_x_vec = arma::zeros<arma::vec> (param);


	
	//calculate kernel matrices
	kernel_mat = kernel_func(X_mat, X_mat, hyperp_vec) + I_train*noise;
	kernel_s_mat = kernel_func(X_mat, X_s_mat, hyperp_vec);
	kernel_ss_mat = kernel_func(X_s_mat, X_s_mat, hyperp_vec);

	//numerical stability factor in solving the Cholesky Decomposition
	kernel_mat += I_train*epsilon;
	kernel_ss_mat += I_test*epsilon;

	//calculate MEAN of distribution
	mean_mat = kernel_s_mat.t()*kernel_mat.i()*Y_mat;
	//Calculate COVARIANCE of distribution
	var_mat = kernel_ss_mat - kernel_s_mat.t()*kernel_mat.i()*kernel_s_mat;
	//Cholesky Decomposition, 'sqrt' of Variance matrix, here the lower triangular form
	L = arma::chol(var_mat,"lower");

	//generator random distribution
	//create matrix of "Mean" values for easy generation of output function values
	for(i=0;i<samples;i++){
		for(j=0;j<test;j++){
			random_sample(j,i) = dist(generator);
			Mean_mat(j,i) = mean_mat(j,0);
		}
	}

	//sample posterior functions
	post_func = Mean_mat + L*random_sample;

	//generate output matrix:
	//		param1, param2, ... , mean1, var1, mean2, var2, ... , post_func1, post_func2, ...
	for(i=0;i<test;i++){
		for(j=0;j<param;j++){
			output_mat(i,j) = X_s_mat(i,j);
		}
		for(j=0;j<observables;j++){
			output_mat(i,j*2+param) = mean_mat(i,j);
			output_mat(i,j*2+1+param) = var_mat(i,i);
		}
		for(j=0;j<samples;j++){
			output_mat(i,j+param+observables*2) = post_func(i,j);
		}
	}

	return output_mat;
}

/**REGRESSION FUNCTIONS
**********************************************/
arma::mat regression_polynomial_function(arma::mat X_mat, arma::vec parameters){
	int i, j, k,
		p = parameters(0),

		number = X_mat.n_rows,
		dim = X_mat.n_cols;

	arma::mat H = arma::zeros<arma::mat>(p,number);

	arma::vec x_vec = arma::zeros<arma::vec>(dim),
	  value_vec = x_vec;

	for(i=0;i<number;i++){
		for(k=0;k<dim;k++){
			value_vec(k) = 1.0;
		}
		for(j=0;j<p;j++){
			for(k=0;k<dim;k++){
				x_vec(k) = X_mat(i,k);
				H(j,i) += value_vec(k);
				value_vec(k) *= x_vec(k);
			}
		}
	}

	return H;
}
arma::mat regression_linear_function(arma::mat X_mat, arma::mat beta){
  int points = X_mat.n_rows;
  int param = X_mat.n_cols;
  arma::mat H = arma::zeros<arma::mat>(1,points);

  for(int i=0;i<points;i++){
    H(0,i) = beta(0,0);
    for(int j=0;j<param;j++){
      H(0,i) += beta(0,j+1)*X_mat(i,j);
    }
  }
  return H;
}
arma::mat regression_linear_function_ind(arma::mat X_mat, arma::mat beta){
  int points = X_mat.n_rows;
  int param = X_mat.n_cols;


  arma::mat H = arma::zeros<arma::mat>(1,points);
  for(int i=0;i<points;i++){
    for(int j=0;j<param;j++){
      H(0,i) += beta(j,0) + beta(j,1)*X_mat(i,j);
    }
  }
  return H;
}
/**WRITE TO FILE FUNCTIONS
**********************************************/
void write_output(arma::mat output_mat, int test, int param, int observables, int samples, std::string outfilename){
	//write to file as:
	//		param1, param2, ... , mean value, mean-2*std, mean+2*std, output1, output2,....
	int i, j;
	std::ofstream ofile;

	ofile.open(outfilename);
	for(i=0;i<test;i++){
		for(j=0;j<param;j++){
			ofile << " " << output_mat(i,j);
		}
		for(j=0;j<observables;j++){
			ofile << " " << output_mat(i,j*2+param);
			ofile << " " << output_mat(i,j*2+param) - 2.0*sqrt(output_mat(i,j*2+1+param));
			ofile << " " << output_mat(i,j*2+param) + 2.0*sqrt(output_mat(i,j*2+1+param));
		}
		for(j=0;j<samples;j++){
			ofile << " " << output_mat(i,j+param+2*observables);
		}
		ofile << std::endl;
	}
	ofile.close();
}
void write_trainset(arma::mat X_mat, arma::mat Y_mat, std::string outfilename){
	//Write to file as:
	//		param1, param2, ... , training value
	int i, j,
		train = X_mat.n_rows,
		param = X_mat.n_cols,
		observables = Y_mat.n_cols;
	std::ofstream ofile;

	ofile.open(outfilename);
	for(i=0;i<train;i++){
		for(j=0;j<param;j++){
			ofile << " " << X_mat(i,j);
		}
		for(j=0;j<observables;j++){
		  ofile << " " << Y_mat(i,j);
		}
		ofile << std::endl;
	}
	ofile.close();
}
void write_file(std::string fileName, arma::mat &file){
  int write = file.n_rows;
  int param = file.n_cols;
  std::string temp;
  std::ofstream ofile;
  ofile.open(fileName);
  for(int i=0;i<write;i++){
    for(int j=0;j<param;j++){
      ofile << " " <<  file(i,j);
    }
    ofile << std::endl;
  }
  ofile.close();
}

/**SEARCH FUNCTIONS
**********************************************/
int find_max(arma::vec input_vec){
	int i, max_index = 0,
		length = input_vec.n_elem;
	
	double value = input_vec(0),
			max = value;

	for(i=0;i<length;i++){
		value = input_vec(i);
		if(value > max){
			max = value;
			max_index = i;
		}
	}

	return max_index;
}
arma::vec find_zeros(arma::vec input_vec){
	int i, count = 0,
		length = input_vec.n_elem;
	
	double value1 = input_vec(0),
			value2 = value1;

	arma::vec index_vec = arma::zeros<arma::vec>(length);

	for(i=0;i<length;i++){
		value2 = input_vec(i);
		if((value1*value2)<0){
			index_vec(count) = i;
			count++;
		}
		value1 = value2;
	}
	index_vec.resize(count+1);
	index_vec(count)=-1;
	return index_vec;
}
/**NUMERICAL ISSUE FUNCTIONS
**********************************************/
long double power_of_number(long double number){
	long double value=number;
	double power=0;
	while(value<1.0){
			//std::cout << value << ":" << power << std::endl;
			value *= 10.0;
			power++;
	}
	return power;
}
long double base_of_number(long double number){
	long double value=number;
	double power=0;
	while(value<1.0){
			//std::cout << value << ":" << power << std::endl;
			value *= 10.0;
			power++;
	}
	return value;
}
