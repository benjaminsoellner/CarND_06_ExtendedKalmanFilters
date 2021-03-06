#include "FusionEKF.h"
#include "tools.h"
#include "Eigen/Dense"
#include "kalman_filter.h"
#include <iostream>

using namespace std;
using Eigen::MatrixXd;
using Eigen::VectorXd;
using std::vector;

/*
 * Constructor.
 */
FusionEKF::FusionEKF() {
  is_initialized_ = false;

  previous_timestamp_ = 0;

  // initializing matrices
  R_laser_ = MatrixXd(2, 2);
  R_radar_ = MatrixXd(3, 3);
  H_laser_ = MatrixXd(2, 4);

  // measurement function matrix - radar
  Hj_ = MatrixXd(3, 4);

  //measurement covariance matrix - laser
  R_laser_ << 0.0225, 0,
        0, 0.0225;

  //measurement covariance matrix - radar
  R_radar_ << 0.09, 0, 0,
        0, 0.0009, 0,
        0, 0, 0.09;

  /**
    * DONE
    * Initialize the FusionEKF.
    * Set the process and measurement noises
  */

  // measurement function matrix - laser
  H_laser_ << 1, 0, 0, 0,
	  0, 1, 0, 0;
  
  // state covariance matrix
  MatrixXd P_ = MatrixXd(4, 4);
  P_ << 1, 0, 0, 0,
	  0, 1, 0, 0,
	  0, 0, 1000, 0,
	  0, 0, 0, 1000;

  // initial state transition matrix
  MatrixXd F_ = MatrixXd(4, 4);
  F_ << 1, 0, 1, 0,
	  0, 1, 0, 1,
	  0, 0, 1, 0,
	  0, 0, 0, 1;

  // initial covariance matrix including noise
  MatrixXd Q_ = MatrixXd(4, 4);
  Q_ << 1, 0, 1, 0,
	  0, 1, 0, 1,
	  1, 0, 1, 0,
	  0, 1, 0, 1;

  // initial state vector
  VectorXd x_ = VectorXd(4);
  x_ << 1, 1, 1, 1;

  // initialize filter
  ekf_.Init(x_, P_, F_, H_laser_, R_laser_, Q_);

  sigmasq_ax_ = 9.0;
  sigmasq_ay_ = 9.0;
  
}

/**
* Destructor.
*/
FusionEKF::~FusionEKF() {}

void FusionEKF::ProcessMeasurement(const MeasurementPackage &measurement_pack) {


  /*****************************************************************************
   *  Initialization
   ****************************************************************************/
  if (!is_initialized_) {
    /**
      DONE
      * Initialize the state ekf_.x_ with the first measurement.
      * Create the covariance matrix.
      * Remember: you'll need to convert radar from polar to cartesian coordinates.
    */
    // first measurement
    cout << "EKF: " << endl;
    ekf_.x_ = VectorXd(4);
    ekf_.x_ << 1, 1, 1, 1;

	if (measurement_pack.sensor_type_ == MeasurementPackage::RADAR) {
	  // Convert radar from polar to cartesian 
		float rho = measurement_pack.raw_measurements_[0];
		float phi = measurement_pack.raw_measurements_[1];
		ekf_.x_ << rho * cos(phi),
			rho * sin(phi), 
			0, 
			0;
    }
    else if (measurement_pack.sensor_type_ == MeasurementPackage::LASER) {
		ekf_.x_ << measurement_pack.raw_measurements_[0],
			measurement_pack.raw_measurements_[1],
			0,
			0;
    }

	previous_timestamp_ = measurement_pack.timestamp_;

    // done initializing, no need to predict or update
    is_initialized_ = true;
    return;
  }

  /*****************************************************************************
   *  Prediction
   ****************************************************************************/

  /**
   DONE:
     * Update the state transition matrix F according to the new elapsed time.
      - Time is measured in seconds.
     * Update the process noise covariance matrix.
     * Use noise_ax = 9 and noise_ay = 9 for your Q matrix.
   */

  float delta_t = (measurement_pack.timestamp_ - previous_timestamp_) / 1000000.0;
  previous_timestamp_ = measurement_pack.timestamp_;

  // update state transition matrix
  ekf_.F_(0, 2) = delta_t;
  ekf_.F_(1, 3) = delta_t;

  // update state covariance matrix

  float c1 = delta_t * delta_t;
  float c2 = c1 * delta_t / 2;
  float c4 = c2 * delta_t / 2;
  ekf_.Q_ << c4 * sigmasq_ax_, 0, c2 * sigmasq_ax_, 0,
	  0, c4 * sigmasq_ay_, 0, c2 * sigmasq_ay_,
	  c2 * sigmasq_ax_, 0, c1 * sigmasq_ax_, 0,
	  0, c2 * sigmasq_ay_, 0, c1 * sigmasq_ay_;

  ekf_.Predict();

  /*****************************************************************************
   *  Update
   ****************************************************************************/

  /**
   TODO:
     * Use the sensor type to perform the update step.
     * Update the state and covariance matrices.
   */

  if (measurement_pack.sensor_type_ == MeasurementPackage::RADAR) {
	// Radar updates
	ekf_.R_ = R_radar_;
	Hj_ = tools.CalculateJacobian(ekf_.x_);
	ekf_.H_ = Hj_;
	ekf_.UpdateEKF(measurement_pack.raw_measurements_);
  } else {
    // Laser updates
	ekf_.R_ = R_laser_;
	ekf_.H_ = H_laser_;
	ekf_.Update(measurement_pack.raw_measurements_);
  }

  // print the output
  cout << "x_ = " << ekf_.x_ << endl;
  cout << "P_ = " << ekf_.P_ << endl;
}
