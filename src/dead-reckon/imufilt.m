% This is some sample code for doing filtered attitude estimation using
% the Xsens (or any other 3-axis gyro/accel).  As is, this doesn't do
% much better than the XSens's internal attitude filter, however, by
% adding our prior about acceleration from the wheel speeds, it should
% behave very nicely.
%
% t is a vector of timestamps of length N (seconds)
% gyro is a 3xN matrix of gyro readings from the IMU
% accel is a 3xN matrix of accelerometer readings from the IMU
%
% The attitude filter estimates the gyro biases, a vector in the
% direction of gravity, and the rotational rates of the IMU.  It
% doesn't estimate a full quaternion, because yaw has no constraints.
% If you want yaw, you would take the component of rotation rate
% in the plane perpendicular to the gravity vector at every timestep.
% Then, integrate yaw manually from this rate.  You can then combine
% this integrated yaw with the gravity vector to compute a full
% quaternion.
function x = imufilt(t, gyro, accel);

% x is the filter state:
%   x(1:3) are the estimated gyro biases
%   x(4:6) are the estimated rotation rates
%   x(7:9) is a unit vector in the direction of gravity

% Process noise
Q = [eye(3)*0.00001^2 zeros(3)     zeros(3); ...
     zeros(3)         eye(3)*0.01^2 zeros(3); ...
     zeros(3)         zeros(3)      zeros(3)];

% Measurement noise (gyro in upper left, accel in lower right)
R = [eye(3)*0.015^2    zeros(3); ...
     zeros(3)         eye(3)*(3.0/9.8)^2];
% Jacobian of measurement update
H = [eye(3)   eye(3)   zeros(3); ...
     zeros(3) zeros(3) eye(3)];

x = zeros(9, length(t));

% Initialize biases to the first gyro rates
x(:,1) = [gyro(:,1); zeros(3,1); accel(:,1)];

% Initial covariance
P = diag([0.015^2 0.015^2 0.015^2 0 0 0 0.1^2 0.1^2 0.1^2]);

for i = 2:length(t),
    % extract the current biases
    b = x(1:3,i-1);
    % extract the current rotation rates
    w = x(4:6,i-1);
    % extract the current gravity vector
    d = x(7:9,i-1);

    % convert the rotation rates into an angular velocity matrix form
    om = [0 -w(3) w(2); w(3) 0 -w(1); -w(2) w(1) 0];

    % This is using the Rodrigues formula for rotation.  It's more
    % exact, but hard to take the Jacobian of it, so it's currently
    % commented out.
    %th = norm(w)*(t(i)-t(i-1));
    %if th == 0,
    %    mex = eye(3);
    %else
    %    mex = eye(3) + om*sin(th)/th + om*om*2*(sin(th/2)^2 / th^2);
    %end
    %x(7:9,i) = mex*d;
    
    % State update
    x(1:3,i) = b;    % biases stay constant over time
    x(4:6,i) = w;    % rates stay constant over time
    x(7:9,i) = d + om*d*(t(i)-t(i-1));
                     % gravity vector is rotated according to the rotation
                     % rates.

    % Renormalize the direction vector
    x(7:9,i) = x(7:9,i) / norm(x(7:9,i));

    % Compute the Jacobian of the state update
    ds = [0 d(3) -d(2); -d(3) 0 d(1); d(2) -d(1) 0];
    A = [eye(3)   zeros(3)     zeros(3); ...
         zeros(3) eye(3)       zeros(3); ...
         zeros(3) ds*(t(i)-t(i-1))   eye(3)+om*(t(i)-t(i-1))];

    % Update the covariance
    P = A*P*A' + Q;

    % Kalman gain
    K = P*H'*inv(H*P*H' + R);
    % Kalman measurement update
    x(:,i) = x(:,i) + K*([gyro(:,i); accel(:,i)] - H*x(:,i));
    P = (eye(9) - K*H)*P;
end
