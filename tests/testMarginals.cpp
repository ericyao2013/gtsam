/* ----------------------------------------------------------------------------

 * GTSAM Copyright 2010, Georgia Tech Research Corporation, 
 * Atlanta, Georgia 30332-0415
 * All Rights Reserved
 * Authors: Frank Dellaert, et al. (see THANKS for the full author list)

 * See LICENSE for the license information

 * -------------------------------------------------------------------------- */

/**
 * @file testMarginals.cpp
 * @brief 
 * @author Richard Roberts
 * @date May 14, 2012
 */

#include <CppUnitLite/TestHarness.h>

// for all nonlinear keys
#include <gtsam/nonlinear/Symbol.h>

// for points and poses
#include <gtsam/geometry/Point2.h>
#include <gtsam/geometry/Pose2.h>

// for modeling measurement uncertainty - all models included here
#include <gtsam/linear/NoiseModel.h>

// add in headers for specific factors
#include <gtsam/slam/PriorFactor.h>
#include <gtsam/slam/BetweenFactor.h>
#include <gtsam/slam/BearingRangeFactor.h>

#include <gtsam/nonlinear/Marginals.h>

using namespace std;
using namespace gtsam;

TEST(Marginals, planarSLAMmarginals) {

  // Taken from PlanarSLAMSelfContained_advanced

  // create keys for variables
  Symbol x1('x',1), x2('x',2), x3('x',3);
  Symbol l1('l',1), l2('l',2);

  // create graph container and add factors to it
  NonlinearFactorGraph graph;

  /* add prior  */
  // gaussian for prior
  SharedDiagonal prior_model = noiseModel::Diagonal::Sigmas(Vector_(3, 0.3, 0.3, 0.1));
  Pose2 prior_measurement(0.0, 0.0, 0.0); // prior at origin
  graph.add(PriorFactor<Pose2>(x1, prior_measurement, prior_model));  // add the factor to the graph

  /* add odometry */
  // general noisemodel for odometry
  SharedDiagonal odom_model = noiseModel::Diagonal::Sigmas(Vector_(3, 0.2, 0.2, 0.1));
  Pose2 odom_measurement(2.0, 0.0, 0.0); // create a measurement for both factors (the same in this case)
  // create between factors to represent odometry
  graph.add(BetweenFactor<Pose2>(x1, x2, odom_measurement, odom_model));
  graph.add(BetweenFactor<Pose2>(x2, x3, odom_measurement, odom_model));

  /* add measurements */
  // general noisemodel for measurements
  SharedDiagonal meas_model = noiseModel::Diagonal::Sigmas(Vector_(2, 0.1, 0.2));

  // create the measurement values - indices are (pose id, landmark id)
  Rot2 bearing11 = Rot2::fromDegrees(45),
     bearing21 = Rot2::fromDegrees(90),
     bearing32 = Rot2::fromDegrees(90);
  double range11 = sqrt(4.0+4.0),
       range21 = 2.0,
       range32 = 2.0;

  // create bearing/range factors
  graph.add(BearingRangeFactor<Pose2, Point2>(x1, l1, bearing11, range11, meas_model));
  graph.add(BearingRangeFactor<Pose2, Point2>(x2, l1, bearing21, range21, meas_model));
  graph.add(BearingRangeFactor<Pose2, Point2>(x3, l2, bearing32, range32, meas_model));

  // linearization point for marginals
  Values soln;
  soln.insert(x1, Pose2(0.0, 0.0, 0.0));
  soln.insert(x2, Pose2(2.0, 0.0, 0.0));
  soln.insert(x3, Pose2(4.0, 0.0, 0.0));
  soln.insert(l1, Point2(2.0, 2.0));
  soln.insert(l2, Point2(4.0, 2.0));

  Matrix expectedx1(3,3);
  expectedx1 <<
      0.09, -7.1942452e-18, -1.27897692e-17,
      -7.1942452e-18,         0.09, 1.27897692e-17,
      -1.27897692e-17,  1.27897692e-17,         0.01;
  Matrix expectedx2(3,3);
  expectedx2 <<
      0.120967742, -0.00129032258, 0.00451612903,
      -0.00129032258,  0.158387097, 0.0206451613,
      0.00451612903,  0.0206451613, 0.0177419355;
  Matrix expectedx3(3,3);
  expectedx3 <<
      0.160967742, 0.00774193548,  0.00451612903,
      0.00774193548,   0.351935484, 0.0561290323,
      0.00451612903,  0.0561290323, 0.0277419355;
  Matrix expectedl1(2,2);
  expectedl1 <<
      0.168709677, -0.0477419355,
      -0.0477419355,   0.163548387;
  Matrix expectedl2(2,2);
  expectedl2 <<
      0.293870968, -0.104516129,
     -0.104516129,  0.391935484;

  // Check marginals covariances for all variables (Cholesky mode)
  Marginals marginals(graph, soln, Marginals::CHOLESKY);
  EXPECT(assert_equal(expectedx1, marginals.marginalCovariance(x1), 1e-8));
  EXPECT(assert_equal(expectedx2, marginals.marginalCovariance(x2), 1e-8));
  EXPECT(assert_equal(expectedx3, marginals.marginalCovariance(x3), 1e-8));
  EXPECT(assert_equal(expectedl1, marginals.marginalCovariance(l1), 1e-8));
  EXPECT(assert_equal(expectedl2, marginals.marginalCovariance(l2), 1e-8));

  // Check marginals covariances for all variables (QR mode)
  marginals = Marginals(graph, soln, Marginals::QR);
  EXPECT(assert_equal(expectedx1, marginals.marginalCovariance(x1), 1e-8));
  EXPECT(assert_equal(expectedx2, marginals.marginalCovariance(x2), 1e-8));
  EXPECT(assert_equal(expectedx3, marginals.marginalCovariance(x3), 1e-8));
  EXPECT(assert_equal(expectedl1, marginals.marginalCovariance(l1), 1e-8));
  EXPECT(assert_equal(expectedl2, marginals.marginalCovariance(l2), 1e-8));

  // Check joint marginals for 3 variables
  Matrix expected_l2x1x3(8,8);
  expected_l2x1x3 <<
      0.293871159514111,  -0.104516127560770,   0.090000180000270,  -0.000000000000000,  -0.020000000000000,   0.151935669757191,  -0.104516127560770,  -0.050967744878460,
     -0.104516127560770,   0.391935664055174,   0.000000000000000,   0.090000180000270,   0.040000000000000,   0.007741936219615,   0.351935664055174,   0.056129031890193,
      0.090000180000270,   0.000000000000000,   0.090000180000270,  -0.000000000000000,   0.000000000000000,   0.090000180000270,   0.000000000000000,   0.000000000000000,
     -0.000000000000000,   0.090000180000270,  -0.000000000000000,   0.090000180000270,   0.000000000000000,  -0.000000000000000,   0.090000180000270,   0.000000000000000,
     -0.020000000000000,   0.040000000000000,   0.000000000000000,   0.000000000000000,   0.010000000000000,   0.000000000000000,   0.040000000000000,   0.010000000000000,
      0.151935669757191,   0.007741936219615,   0.090000180000270,  -0.000000000000000,   0.000000000000000,   0.160967924878730,   0.007741936219615,   0.004516127560770,
     -0.104516127560770,   0.351935664055174,   0.000000000000000,   0.090000180000270,   0.040000000000000,   0.007741936219615,   0.351935664055174,   0.056129031890193,
     -0.050967744878460,   0.056129031890193,   0.000000000000000,   0.000000000000000,   0.010000000000000,   0.004516127560770,   0.056129031890193,   0.027741936219615;
  vector<Key> variables(3);
  variables[0] = l2;
  variables[1] = x1;
  variables[2] = x3;
  JointMarginal joint_l2x1x3 = marginals.jointMarginalCovariance(variables);
  EXPECT(assert_equal(Matrix(expected_l2x1x3.block(0,0,2,2)), Matrix(joint_l2x1x3(l2,l2)), 1e-6));
  EXPECT(assert_equal(Matrix(expected_l2x1x3.block(2,0,3,2)), Matrix(joint_l2x1x3(x1,l2)), 1e-6));
  EXPECT(assert_equal(Matrix(expected_l2x1x3.block(5,0,3,2)), Matrix(joint_l2x1x3(x3,l2)), 1e-6));

  EXPECT(assert_equal(Matrix(expected_l2x1x3.block(0,2,2,3)), Matrix(joint_l2x1x3(l2,x1)), 1e-6));
  EXPECT(assert_equal(Matrix(expected_l2x1x3.block(2,2,3,3)), Matrix(joint_l2x1x3(x1,x1)), 1e-6));
  EXPECT(assert_equal(Matrix(expected_l2x1x3.block(5,2,3,3)), Matrix(joint_l2x1x3(x3,x1)), 1e-6));

  EXPECT(assert_equal(Matrix(expected_l2x1x3.block(0,5,2,3)), Matrix(joint_l2x1x3(l2,x3)), 1e-6));
  EXPECT(assert_equal(Matrix(expected_l2x1x3.block(2,5,3,3)), Matrix(joint_l2x1x3(x1,x3)), 1e-6));
  EXPECT(assert_equal(Matrix(expected_l2x1x3.block(5,5,3,3)), Matrix(joint_l2x1x3(x3,x3)), 1e-6));

  // Check joint marginals for 2 variables (different code path than >2 variable case above)
  Matrix expected_l2x1(5,5);
  expected_l2x1 <<
      0.293871159514111,  -0.104516127560770,   0.090000180000270,  -0.000000000000000,  -0.020000000000000,
     -0.104516127560770,   0.391935664055174,   0.000000000000000,   0.090000180000270,   0.040000000000000,
      0.090000180000270,   0.000000000000000,   0.090000180000270,  -0.000000000000000,   0.000000000000000,
     -0.000000000000000,   0.090000180000270,  -0.000000000000000,   0.090000180000270,   0.000000000000000,
     -0.020000000000000,   0.040000000000000,   0.000000000000000,   0.000000000000000,   0.010000000000000;
  variables.resize(2);
  variables[0] = l2;
  variables[1] = x1;
  JointMarginal joint_l2x1 = marginals.jointMarginalCovariance(variables);
  EXPECT(assert_equal(Matrix(expected_l2x1.block(0,0,2,2)), Matrix(joint_l2x1(l2,l2)), 1e-6));
  EXPECT(assert_equal(Matrix(expected_l2x1.block(2,0,3,2)), Matrix(joint_l2x1(x1,l2)), 1e-6));
  EXPECT(assert_equal(Matrix(expected_l2x1.block(0,2,2,3)), Matrix(joint_l2x1(l2,x1)), 1e-6));
  EXPECT(assert_equal(Matrix(expected_l2x1.block(2,2,3,3)), Matrix(joint_l2x1(x1,x1)), 1e-6));

  // Check joint marginal for 1 variable (different code path than >1 variable cases above)
  variables.resize(1);
  variables[0] = x1;
  JointMarginal joint_x1 = marginals.jointMarginalCovariance(variables);
  EXPECT(assert_equal(expectedx1, Matrix(joint_l2x1(x1,x1)), 1e-6));
}



/* ************************************************************************* */
int main() { TestResult tr; return TestRegistry::runAllTests(tr);}
/* ************************************************************************* */
