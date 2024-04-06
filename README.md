# Compute Web App

<p align="center">
  <img src="banner.png" alt="CWA banner" width="300"/>
</p>

Compute Web App (or CWA) is a web application designed to schedule and run data intensive computation processes.

Its main features:

- Fast and easy to deploy,
- Secured process parameters, data and - results transfer,
- Simple RESTfull API designed to:
  - upload data,
  - control and monitor processes and
  - retrieve results,
- Simple and efficient web interface provided,
- Designed to run processes in parallel,
- Adapted to dedicated servers.

Its dependencies:
- [HDF5](https://www.hdfgroup.org/solutions/hdf5/) for data storage,
- [Eigen](https://eigen.tuxfamily.org) for data arrangment,
- [POCO C++ Libraries](https://pocoproject.org) for process launch and handle,
- [Restbed](https://github.com/Corvusoft/restbed) or [Pistache](https://github.com/pistacheio/pistache) for the RESTful API,
- [Vue.js](https://vuejs.org) for web UI.
