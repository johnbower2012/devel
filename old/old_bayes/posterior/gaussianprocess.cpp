#include "gaussianprocess.hpp"


/**KERNAL FUCNTIONS
**********************************************/

//calculate the kernal of vector sets A and B, using hyperparameters in hyperp, given each has dimensionality param,
//where A and B are the form C = [X1,X2,...] where each Xi.t = [param_i_1, param_i_2, ...].
arma::mat kernal_square_exp_noise_function(arma::mat A, arma::mat B, arma::vec hyperp_vec){
	double xi, xj, sum, diff,
			theta1, theta2, theta3;
	int i, j, k,
		ai = A.n_rows, bi = B.n_rows,
		param = A.n_cols;

	arma::mat kernal_matrix = arma::zeros<arma::mat> (ai, bi);

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
				kernal_matrix(i,j) = theta1*exp(-sum*theta2);
			}
		}
		for(i=0;i<bi;i++){
			kernal_matrix(i,i) += theta3;
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
				kernal_matrix(i,j) = theta1*exp(-sum*theta2);
			}
			kernal_matrix(i,i) += theta3;
		}
	}

	return kernal_matrix;
}
arma::mat kernal_derivative_function(arma::mat kernal_func(arma::mat, arma::mat, arma::vec), arma::mat A, arma::mat B, arma::vec hyperp_vec, int selection, double precision){
	int i, j,
		a = A.n_rows,
		b = B.n_cols;

	arma::vec 	temp_plus = hyperp_vec,
				temp_minus = hyperp_vec;

	arma::mat 	kernal_plus = arma::zeros<arma::mat>(a,b),
				kernal_minus = arma::zeros<arma::mat>(a,b);

	temp_plus = temp_minus = hyperp_vec;

	temp_plus(selection) += precision;
	temp_minus(selection) -= precision;

	kernal_plus = kernal_func(A,B,temp_plus);
	kernal_minus = kernal_func(A,B,temp_minus);

	return (kernal_plus - kernal_minus)/(2.0*precision);
}
arma::mat kernal_square_exp_function(arma::mat A, arma::mat B, arma::vec hyperp_vec){
	double xi, xj, sum, diff,
			theta1, theta2;
	int i, j, k,
		ai = A.n_rows, bi = B.n_rows,
		param = A.n_cols;

	arma::mat kernal_matrix = arma::zeros<arma::mat> (ai, bi);

	theta1 = hyperp_vec(0)*hyperp_vec(0);
	theta2 = 1.0/hyperp_vec(1)/hyperp_vec(1)/2.0;

	for(i=0;i<ai;i++){
		for(j=0;j<bi;j++){
			sum = 0.0;
			for(k=0;k<param;k++){
				xi = A(i,k);
				xj = B(j,k);
				diff = xi - xj;
				sum += diff*diff*theta2;
			}
			kernal_matrix(i,j) = theta1*exp(-sum);
		}
	}

	return kernal_matrix;
}
arma::mat kernal_periodic_decayaway_function(arma::mat A, arma::mat B, arma::vec hyperp_vec){
	double xi, xj, sum, diff,
			theta1, theta2, theta3,
			pi = acos(-1.0), sine;
	int i, j, k,
		ai = A.n_rows, bi = B.n_rows,
		param = A.n_cols;

	arma::mat kernal_matrix = arma::zeros<arma::mat> (ai, bi);

	theta1 = hyperp_vec(0)*hyperp_vec(0);
	theta2 = 1.0/hyperp_vec(1)/hyperp_vec(1)/2.0;
	theta3 = 2.0/hyperp_vec(2)/hyperp_vec(2);

	for(i=0;i<ai;i++){
		for(j=0;j<bi;j++){
			sum = 0.0;
			for(k=0;k<param;k++){
				xi = A(i,k);
				xj = B(j,k);
				diff = xi - xj;
				sine = std::sin(pi*diff);
				sum += diff*diff*theta2 + sine*sine*theta3;
			}
			kernal_matrix(i,j) = theta1*exp(-sum);
		}
	}

	return kernal_matrix;
}
arma::mat kernal_rational_quadratic_function(arma::mat A, arma::mat B, arma::vec hyperp_vec){
	double xi, xj, sum, diff,
			theta1, theta2, theta3;
	int i, j, k,
		ai = A.n_rows, bi = B.n_rows,
		param = A.n_cols;

	arma::mat kernal_matrix = arma::zeros<arma::mat> (ai, bi);

	theta1 = hyperp_vec(0)*hyperp_vec(0);
	theta2 = 1.0/hyperp_vec(1)/hyperp_vec(1)/2.0;
	theta3 = hyperp_vec(2);

	for(i=0;i<ai;i++){
		for(j=0;j<bi;j++){
			sum = 0.0;
			for(k=0;k<param;k++){
				xi = A(i,k);
				xj = B(j,k);
				diff = xi - xj;
				sum += pow((1.0 + diff*diff*theta2/theta3),-theta3);
			}
			kernal_matrix(i,j) = theta1*(-sum);
		}
	}

	return kernal_matrix;
}
arma::mat kernal_co2_function(arma::mat A, arma::mat B, arma::vec hyperp_vec){
	double xi, xj, sum, diff;
	int i, j, k,
		ai = A.n_rows, bi = B.n_rows,
		param = A.n_cols;

	arma::mat kernal_matrix = arma::zeros<arma::mat> (ai, bi);

	arma::vec 	hyperp_1 = arma::zeros<arma::vec>(3),
				hyperp_2 = hyperp_1,																																																																																																																																																																																																						
				hyperp_3 = hyperp_1,
				hyperp_4 = hyperp_1;

	hyperp_1(0) = hyperp_vec(0);
	hyperp_1(1) = hyperp_vec(1);

	hyperp_2(0) = hyperp_vec(2);
	hyperp_2(1) = hyperp_vec(3);
	hyperp_2(2) = hyperp_vec(4);

	hyperp_3(0) = hyperp_vec(5);
	hyperp_3(1) = hyperp_vec(6);
	hyperp_3(2) = hyperp_vec(7);

	hyperp_4(0) = hyperp_vec(8);
	hyperp_4(1) = hyperp_vec(9);
	hyperp_4(2) = hyperp_vec(10);

	kernal_matrix = kernal_square_exp_function(A,B,hyperp_1);
	kernal_matrix += kernal_periodic_decayaway_function(A,B,hyperp_2);
	kernal_matrix += kernal_rational_quadratic_function(A,B,hyperp_3);
	kernal_matrix += kernal_square_exp_noise_function(A,B,hyperp_4);

	return kernal_matrix;
}


/**GAUSSIAN PROCESS FUNCTIONS
**********************************************/

arma::mat gaussian_process_solver_regression(
	arma::mat kernal_func(arma::mat, arma::mat, arma::vec), arma::mat X_mat, arma::mat Y_mat, arma::mat X_s_mat, 
	int samples, arma::vec hyperp_vec, double epsilon, 
	arma::mat regression_func(arma::mat, arma::vec), int power)
	{
	//random number generator
	unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
	std::default_random_engine generator(seed);
	std::normal_distribution<double> dist(0,1.0);

	int i, j,
		train = X_mat.n_rows, test = X_s_mat.n_rows,
		param = X_mat.n_cols, observables = Y_mat.n_cols,
		hyperp = hyperp_vec.n_elem;

	//armadillo matrices and vectors for computational use
	arma::mat 	kernal_mat = arma::zeros<arma::mat>(train,train),
				kernal_inv_mat = kernal_mat,
				kernal_s_mat = arma::zeros<arma::mat>(train,test),
				kernal_ss_mat = arma::zeros<arma::mat>(test,test),

				mean_mat = arma::zeros<arma::mat>(test,observables),
				var_mat = kernal_ss_mat,
				L = var_mat,

				H_mat = arma::zeros<arma::mat>(power,train),
				H_s_mat = arma::zeros<arma::mat>(power,test),
				R_mat = arma::zeros<arma::mat>(train,test),
				beta_mat = arma::zeros<arma::mat>(train,observables),

				temp_pt = arma::zeros<arma::mat>(power,test),
				temp_pp = arma::zeros<arma::mat>(power,power),
				
				post_func = arma::zeros<arma::mat>(test,samples),
				random_sample = post_func,
				Mean_mat = post_func,

				I_train = arma::eye<arma::mat>(train,train),

				output_mat = arma::zeros<arma::mat>(test, param + 2*observables + samples);

	arma::vec	param_x_vec = arma::zeros<arma::vec> (param),
				param_vec = arma::zeros<arma::vec> (1);

	param_vec(0) = power;

	//calculate kernal matrices
	kernal_mat = kernal_func(X_mat, X_mat, hyperp_vec);
	kernal_s_mat = kernal_func(X_mat, X_s_mat, hyperp_vec);
	kernal_ss_mat = kernal_func(X_s_mat, X_s_mat, hyperp_vec);

	//numerical stability factor in solving the Cholesky Decomposition
	kernal_mat += I_train*epsilon;
	kernal_inv_mat = kernal_mat.i();

	//calculate regression related matrices R and beta
	H_mat = regression_func(X_mat, param_vec);
	H_s_mat = regression_func(X_s_mat, param_vec);

	R_mat = H_s_mat - H_mat*kernal_inv_mat*kernal_s_mat;

	temp_pp = H_mat*kernal_inv_mat*H_mat.t();
	beta_mat = temp_pp.i()*H_mat*kernal_inv_mat*Y_mat;

	//calculate MEAN of distribution
	mean_mat = kernal_s_mat.t()*kernal_mat.i()*Y_mat + R_mat.t()*beta_mat;
	//Calculate COVARIANCE of distribution
	var_mat = kernal_ss_mat - kernal_s_mat.t()*kernal_mat.i()*kernal_s_mat + R_mat.t()*temp_pp.i()*R_mat;	
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

arma::mat gaussian_process_solver_basic(arma::mat kernal_func(arma::mat, arma::mat, arma::vec), arma::mat X_mat, arma::mat Y_mat, arma::mat X_s_mat, int samples, arma::vec hyperp_vec, double epsilon){
		//random number generator
	unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
	std::default_random_engine generator(seed);
	std::normal_distribution<double> dist(0,1.0);

	int i, j,
		train = X_mat.n_rows, test = X_s_mat.n_rows,
		param = X_mat.n_cols, observables = Y_mat.n_cols,
		hyperp = hyperp_vec.n_elem;

	//armadillo matrices and vectors for computational use
	arma::mat 	kernal_mat = arma::zeros<arma::mat>(train,train),
				kernal_inv_mat = kernal_mat,
				kernal_s_mat = arma::zeros<arma::mat>(train,test),
				kernal_ss_mat = arma::zeros<arma::mat>(test,test),

				mean_mat = arma::zeros<arma::mat>(test,observables),
				var_mat = kernal_ss_mat,
				L = var_mat,
				
				post_func = arma::zeros<arma::mat>(test,samples),
				random_sample = post_func,
				Mean_mat = post_func,

				I_train = arma::eye<arma::mat>(train,train),

				output_mat = arma::zeros<arma::mat>(test, param + 2*observables + samples);

	arma::vec	param_x_vec = arma::zeros<arma::vec> (param),
				param_vec = arma::zeros<arma::vec> (1);

	//calculate kernal matrices
	kernal_mat = kernal_func(X_mat, X_mat, hyperp_vec);
	kernal_s_mat = kernal_func(X_mat, X_s_mat, hyperp_vec);
	kernal_ss_mat = kernal_func(X_s_mat, X_s_mat, hyperp_vec);

	//numerical stability factor in solving the Cholesky Decomposition
	kernal_mat += I_train*epsilon;
	kernal_inv_mat = kernal_mat.i();

	//calculate MEAN of distribution
	mean_mat = kernal_s_mat.t()*kernal_mat.i()*Y_mat;
	//Calculate COVARIANCE of distribution
	var_mat = kernal_ss_mat - kernal_s_mat.t()*kernal_mat.i()*kernal_s_mat;	
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

	double x, value;

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
			ofile << std::setw(15) << output_mat(i,j);
		}
		for(j=0;j<observables;j++){
			ofile << std::setw(15) << output_mat(i,j*2+param);
			ofile << std::setw(15) << output_mat(i,j*2+param) - 2.0*sqrt(output_mat(i,j*2+1+param));
			ofile << std::setw(15) << output_mat(i,j*2+param) + 2.0*sqrt(output_mat(i,j*2+1+param));
		}
		for(j=0;j<samples;j++){
			ofile << std::setw(15) << output_mat(i,j+param+2*observables);
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
int find_zero(arma::vec input_vec){
	int i, zero_index = 0, count = 0,
		length = input_vec.n_elem;
	
	double value1 = input_vec(0),
			value2 = value1,
			zero = value2;

	for(i=0;i<length;i++){
		value2 = input_vec(i);
		if((value1*value2)<0){
			zero = value2;
			zero_index = i;
			count++;
		}
		value1 = value2;
	}

	return zero_index;
}

/**LIKELIHOOD FUNCTIONS
**********************************************/
arma::vec log_likelihood_function(arma::mat Y_mat, arma::mat X_mat, int resolution, arma::mat kernal_func(arma::mat, arma::mat, arma::vec), arma::vec hyperp_vec, arma::mat hyperp_range_vec, int selection){
	int i, j,
		train = X_mat.n_rows,
		param = X_mat.n_cols,
		hyperp = hyperp_vec.n_elem;

	double value=0,
			theta_i = hyperp_range_vec(0, selection),
			theta_f = hyperp_range_vec(1, selection),
			dtheta = (theta_f - theta_i)/(double) (resolution - 1);

	arma::mat 	kernal_mat = arma::zeros<arma::mat> (train,train),
				kernal_inv_mat = arma::zeros<arma::mat> (train,train),
				value_mat = arma::zeros<arma::mat> (1,1),
				I = arma::eye<arma::mat>(train,train);

	arma::vec	hyperp_temp_vec = arma::zeros<arma::vec>(hyperp),
				log_likelihood_vec = arma::zeros<arma::mat> (resolution);

	for(i=0;i<resolution;i++){
		for(j=0;j<hyperp;j++){
			hyperp_temp_vec(j) = hyperp_vec(j);
		}
		hyperp_temp_vec(selection) = theta_i + dtheta*(double) i;

		kernal_mat = kernal_func(X_mat, X_mat, hyperp_temp_vec);
		kernal_mat += I*1e-8;
		kernal_inv_mat = kernal_mat.i();

		value_mat = -0.5*Y_mat.t()*kernal_inv_mat*Y_mat;
		log_likelihood_vec(i) = value_mat(0,0);
		log_likelihood_vec(i) += -0.5*log(arma::det(kernal_mat));
	}

	return log_likelihood_vec;
}
arma::vec log_likelihood_derivative(arma::mat Y_mat, arma::mat X_mat, int resolution, arma::mat kernal_func(arma::mat, arma::mat, arma::vec), arma::vec hyperp_vec, arma::mat hyperp_range_vec, int selection, double precision){
	int i, j,
		train = X_mat.n_rows,
		param = X_mat.n_cols,
		observables = Y_mat.n_cols,
		hyperp = hyperp_vec.n_elem;

	double value=0,
			theta_i = hyperp_range_vec(0, selection),
			theta_f = hyperp_range_vec(1, selection),
			dtheta = (theta_f - theta_i)/(double) (resolution - 1);

	arma::mat 	kernal_mat = arma::zeros<arma::mat> (train,train),
				kernal_inv_mat = kernal_mat,
				kernal_derivative_mat = kernal_mat,
				I = arma::eye<arma::mat>(train,train),
				alpha = arma::zeros<arma::mat>(train,observables);

	arma::vec	hyperp_temp_vec = arma::zeros<arma::vec>(hyperp),
				temp_plus = hyperp_temp_vec,
				temp_minus = hyperp_temp_vec,
				log_likelihood_der_vec = arma::zeros<arma::mat> (resolution);

	for(i=0;i<resolution;i++){
		hyperp_temp_vec = temp_plus = temp_minus = hyperp_vec;
		hyperp_temp_vec(selection) = theta_i + dtheta*(double) i;
		temp_plus(selection) += precision;
		temp_minus(selection) -= precision;

		kernal_mat = kernal_func(X_mat, X_mat, hyperp_temp_vec);
		kernal_mat += I*1e-8;
		kernal_inv_mat = kernal_mat.i();

		alpha = kernal_inv_mat*Y_mat;

		kernal_derivative_mat = kernal_derivative_function(kernal_func, X_mat, X_mat, hyperp_vec, selection, precision);

		value = 0.5*arma::trace((alpha*alpha.t() - kernal_inv_mat)*kernal_derivative_mat);
		log_likelihood_der_vec(i) = value;
	}

	return log_likelihood_der_vec;
}
