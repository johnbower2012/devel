#include<iostream>
#include<cmath>
#include<iomanip>
#include<random>
#include<armadillo>

/******************** 
  CLASS DECLARATION
 ********************/
class particle{
public:
  int dimensions;
  double mass;
  double charge;
  arma::vec position;
  arma::vec velocity;

  particle();
  particle(int Dimensions);
  particle(int Dimensions, double Mass, double Charge, arma::vec Position, arma::vec Velocity);
  
  void update(arma::vec position, arma::vec velocity);
  void print();
};

class particle_system{
public:
  int particle_count; //number of particles
  int dimensions; //variable count
  int observables; //stats to be collected
  double size; //periodic boundary

  double delta_t; //time step + variance of langevin force
  double gamma; //drag factor
  double temp; //temperature
  double sigma; //sqrt(2*m*T*gamma*delta_t)

  std::vector<particle> system; //the system to be evolved
  arma::mat position; //position of system particles, used for verlet
  arma::mat velocity; //velocity of system particles, used for verlet
  arma::mat statistics; //average momentum, position

  particle_system(int Dimensions, double Size, double Delta_t, double Gamma, double Temp); //constructor
  particle_system(int Dimensions, double Size, double Delta_t, double Gamma, double Temp, int Particles); //auto-constructor  
  
  void add_periodic(particle new_particle); //add particle & convert positions to periodic boundary
  arma::vec relative_coordinate_periodic(); //calc relative coordinates to periodic boundary
  arma::vec langevin_force(); //add langevin force
  arma::vec electrostatic_force_periodic(); //basic repulsive force to periodic boundary

  void verlet_periodic(int steps, bool repulsion, int print_checks); //evolve one time step using periodic

  void statistics_run();

  void print(); //print system information
};

class map_system{
public:
  int count;
  double size;
  arma::mat map;

  map_system(double Size);

  void add(double min, double max);
  double map_x_to_y(double x, int n);
  arma::mat map_x_to_y(arma::mat samples);
};

/******************** 
  MAIN FUNCTION
 ********************/
std::ofstream ofile;

int main(int argc, char* argv[]){
  //ARRANGE DETAILS
  int steps=0;
  int dimensions=2;
  int particles=1000;
  int print_checks=0.0;
  
  double size=10.0;
  double delta_t=0.1;
  double gamma=0.4;
  double temp=200;
  double mass=1.0;
  double charge=1.0;

  bool repulsion=true;

  std::string filename = "sample.dat";

  arma::mat sample_matrix = arma::zeros<arma::mat>(dimensions+1,particles);

  //CHECK FOR PROPER USAGE
  if(argc!=2){
    printf("Improper usage. Enter also 'steps' on same line.\n");
    exit(1);
  } else{
    steps=atoi(argv[1]);
    print_checks=steps/10;
    if(print_checks<=0.0){
      print_checks=1.0;
    }
    //print_checks=0.0;
  }
  
  //BEGIN DEFINITIONS
  particle_system system(dimensions,size,delta_t,gamma,temp,particles);

  printf("Steps: %d\n",steps);
  system.verlet_periodic(steps,repulsion,print_checks);

  system.statistics.print("----------\nending statistics:");  
  system.position.print("----------\nending position:");
  system.velocity.print("----------\nending velocity:");

  sample_matrix = system.position;
  sample_matrix.resize(dimensions,particles);

  map_system map(size);
  for(int i=0;i<dimensions;i++){
    map.add(0,(double) i + 5);
  }
  map.map.print("MAP:");
  sample_matrix = map.map_x_to_y(sample_matrix);
  sample_matrix.print("MAPPED:");

  ofile.open(filename);
  for(int i=0;i<particles;i++){
    for(int j=0;j<dimensions;j++){
      ofile << " " << sample_matrix(j,i);
    }
    ofile << std::endl;
  }
  ofile.close();

  return 0;
}

/******************** 
  CLASS FUNCTIONS
 ********************/
/******************** 
  particle
 ********************/
particle::particle(){
  this->dimensions = 0;
  this->mass = 0;
  this->charge = 0;
  this->position = arma::zeros<arma::vec>(0);
  this->velocity = arma::zeros<arma::vec>(0);
}
particle::particle(int Dimensions){
  this->dimensions = Dimensions;
  this->mass = 1.0;
  this->charge = 0.1;
  this->position = arma::zeros<arma::vec>(dimensions+1);
  this->velocity = arma::zeros<arma::vec>(dimensions+1);
}
particle::particle(int Dimensions, double Mass, double Charge, arma::vec Position, arma::vec Velocity){
  this->dimensions = Dimensions;
  this->mass = Mass;
  this->charge = Charge;
  this->position = Position;
  this->velocity = Velocity;
}
void particle::update(arma::vec Position, arma::vec Velocity){
  this->position = Position;
  this->velocity = Velocity;
}
void particle::print(){
  printf("dim: %d\ncharge: %f\n",this->dimensions,this->charge);
  this->position.print("position");
  this->velocity.print("velocity");
}

/******************** 
  particle_system
 ********************/

particle_system::particle_system(int Dimensions, double Size, double Delta_t, double Gamma, double Temp){
  this->dimensions = Dimensions;
  this->size = Size;
  this->delta_t = Delta_t;
  this->gamma = Gamma;
  this->temp = Temp;
  this->particle_count=0;
  this->sigma=sqrt(2*temp*gamma*delta_t);
  this->observables=3+2*Dimensions;
  this->statistics = arma::zeros<arma::mat>(2,observables);
}
particle_system::particle_system(int Dimensions, double Size, double Delta_t, double Gamma, double Temp, int Particles){
  int particles=Particles;
  this->dimensions = Dimensions;
  this->size = Size;
  this->delta_t = Delta_t;
  this->gamma = Gamma;
  this->temp = Temp;
  this->particle_count=0;
  this->sigma=sqrt(2*temp*dimensions*gamma*delta_t);
  this->observables=3+2*Dimensions;
  this->statistics = arma::zeros<arma::mat>(2,observables);
  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  std::default_random_engine generator (seed);
  std::uniform_real_distribution<double> uni_distribution(0,size);
  std::normal_distribution<double> norm_distribution(0.0,size/10.0);

  double x=0.0,v=0.0;
  particle part(dimensions);
  for(int i=0;i<particles;i++){
    x=0.0;
    v=0.0;
    for(int j=0;j<dimensions;j++){
      part.position(j) = uni_distribution(generator);
      part.velocity(j) = norm_distribution(generator);
      x += part.position(j)*part.position(j);
      v += part.velocity(j)*part.velocity(j);
    }
    part.position(dimensions) = sqrt(x);
    part.velocity(dimensions) = sqrt(v);
    add_periodic(part);
  }

}
void particle_system::add_periodic(particle new_particle){
  double x=0.0,v=0.0;
  this->system.push_back(new_particle);
  for(int i=0;i<dimensions;i++){
    system[particle_count].position(i) = std::fmod(new_particle.position(i),size);
    while(system[particle_count].position(i) < 0){
      system[particle_count].position(i) += size;
    }
    while(system[particle_count].position(i) > size){
      system[particle_count].position(i) -= size;
    }
    system[particle_count].velocity(i) = new_particle.velocity(i);

    x += system[particle_count].position(i)*system[particle_count].position(i);
    v += system[particle_count].velocity(i)*system[particle_count].velocity(i);
  }
  system[particle_count].position(dimensions) = sqrt(x);
  system[particle_count].velocity(dimensions) = sqrt(v);
  
  this->position.resize(dimensions+1,particle_count+1);
  this->position.col(particle_count) = system[particle_count].position;
  this->velocity.resize(dimensions+1,particle_count+1);
  this->velocity.col(particle_count) = new_particle.velocity;
  this->particle_count++;
}
arma::vec particle_system::relative_coordinate_periodic(){
  int index=0;
  int dim=this->dimensions+1;
  double r2=0.0,pi=0.0,pj=0.0,magnitude=0.0,period_mag=0.0;
  arma::vec relcoord = arma::zeros<arma::vec>(particle_count*(particle_count-1)*dim/2);
  for(int i=0;i<this->particle_count;i++){
    for(int j=0;j<i;j++){
      r2=0.0;
      for(int k=0;k<dim;k++){
	index = (i*(i-1)/2 + j)*(dim) + k;
	if(k==dim-1){
	  relcoord(index) = sqrt(r2);
	  continue;
	}
	else{
	  pi = position(k,i);
	  pj = position(k,j);
	  magnitude = fabs(pi-pj);
	  period_mag = size - magnitude;
	  if(magnitude < period_mag){
	    relcoord(index) = position(k,i) - position(k,j);
	  }else{
	    if(pi > pj){
	      relcoord(index) = pi - (size + pj);
	    }else{
	      relcoord(index) = (size + pi) - pj;
	    }
	  }
	  r2 += relcoord(index)*relcoord(index);
	}
      }
    }       
  }
  return relcoord;
}
arma::vec particle_system::langevin_force(){
  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  std::default_random_engine generator (seed);
  std::normal_distribution<double> distribution(0.0,1.0);
  arma::vec force = arma::zeros<arma::vec>(particle_count*dimensions);
  for(int i=0;i<particle_count;i++){
    for(int j=0;j<dimensions;j++){
      force(i*dimensions+j) = -gamma*system[i].mass*velocity(j,i);
      force(i*dimensions+j) += sigma*distribution(generator);
    }
  }
  return force;
}
arma::vec particle_system::electrostatic_force_periodic(){
  int index=0;
  int index_r;
  int dim=this->dimensions+1;
  double force_ij=0.0;
  double r3=0.0;
  arma::vec relcoord = relative_coordinate_periodic();
  arma::vec force = arma::zeros<arma::vec>(particle_count*dimensions);
  for(int i=0;i<particle_count;i++){
    for(int j=0;j<i;j++){
      index_r = (i*(i-1)/2 + j)*dim + dim-1;
      r3 = relcoord(index_r)*relcoord(index_r)*relcoord(index_r);
      for(int k=0;k<dimensions;k++){
	index = (i*(i-1)/2 + j)*(dim) + k;
	force_ij = system[i].charge*system[j].charge*relcoord(index)/r3;
	force(i*dimensions+k) += force_ij;
	force(j*dimensions+k) += -force_ij;
      }
    }
  }
  return force;
}
void particle_system::verlet_periodic(int steps,bool repulsion, int print_checks){
  double half_delta_t = delta_t/2.0;
  double half_delta_tsq = delta_t*half_delta_t;
  double acc_jk,x,v;
  int dim = this->dimensions+1;
  arma::mat print_stat = statistics;
  arma::vec force = arma::zeros<arma::vec>(particle_count*dimensions);
  arma::vec relcoord = arma::zeros<arma::vec>(particle_count*(particle_count-1)/2*dim);
  statistics = arma::zeros<arma::mat>(2,observables);
  if(repulsion==true){
    force = electrostatic_force_periodic();
    force += langevin_force();
  } else{
    force = langevin_force();
  }
  for(int i=0;i<steps;i++){
    for(int j=0;j<particle_count;j++){
      x=0.0;
      for(int k=0;k<dimensions;k++){
	acc_jk = force(j*dimensions+k)/system[j].mass;
	position(k,j) = position(k,j) + delta_t*velocity(k,j) + half_delta_tsq*acc_jk;
	position(k,j) = std::fmod(position(k,j),size);
	while(position(k,j) > size){
	  position(k,j) -= size;
	}
	while(position(k,j) < 0){
	  position(k,j) += size;
	}
	x += position(k,j)*position(k,j);
	velocity(k,j) = velocity(k,j) + half_delta_t*acc_jk;
      }
      position(dimensions,j) = sqrt(x);
    }
    if(repulsion==true){
      force = electrostatic_force_periodic();
      force += langevin_force();
    } else{
      force = langevin_force();
    }
    for(int j=0;j<particle_count;j++){
      v=0.0;
      for(int k=0;k<dimensions;k++){
	acc_jk = force(j*dimensions+k)/system[j].mass;
	velocity(k,j) += half_delta_t*acc_jk;
	v += velocity(k,j)*velocity(k,j);
      }
      velocity(dimensions,j) = sqrt(v);
    }
    statistics_run();
    if(print_checks!=0){
      if((i%print_checks)==0){
	printf("----------\nstep: %d\n",i);
      }
    }
}
  for(int i=0;i<observables;i++){
    statistics(0,i) /= (double) (steps*particle_count);
    statistics(1,i) /= (double) (steps*particle_count);
    statistics(1,i) = sqrt(statistics(1,i)-statistics(0,i)*statistics(0,i));
  }
}
void particle_system::statistics_run(){
  double momentum=0.0,mass=0.0;
  for(int i=0;i<particle_count;i++){
    mass = system[i].mass;
    momentum += mass*mass*velocity(dimensions,i)*velocity(dimensions,i);
    statistics(0,0) += momentum;
    statistics(1,0) += momentum*momentum;
    for(int j=0;j<dimensions+1;j++){
      statistics(0,j+1) += position(j,i);
      statistics(1,j+1) += position(j,i)*position(j,i);
      statistics(0,j+2+dimensions) += velocity(j,i);
      statistics(1,j+2+dimensions) += velocity(j,i)*velocity(j,i);
    }
  }
}
void particle_system::print(){
  for(int i=0;i<particle_count;i++){
    printf("-----Particle%d:------\n",i);
    this->system[i].print();
  }
}

/******************** 
  map
 ********************/
map_system::map_system(double Size){
  this->size = Size;
}
void map_system::add(double min, double max){
  map.resize(3,count+1);
  map(0,count)=min;
  map(1,count)=max;
  map(2,count)=(max-min)/size;
  count++;
}
double map_system::map_x_to_y(double x, int n){
  return x*map(2,n) + map(0,n);
}
arma::mat map_system::map_x_to_y(arma::mat samples){
  int dimensions = samples.n_rows;
  int particles = samples.n_cols;
  arma::mat output = arma::zeros<arma::mat>(dimensions,particles);
  for(int i=0;i<dimensions;i++){
    for(int j=0;j<particles;j++){
      output(i,j) = samples(i,j)*map(2,i) + map(0,i);
    }
  }
  return output;
}
