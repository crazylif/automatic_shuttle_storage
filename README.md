// run Node Red
cmd:
  node-red

// run broker (mosquitto)
note:
  cd to mosquitto file, then change ip on mosquitto.conf file to correct ip address
cmd:
  mosquitto -v -c mosquitto.conf

// check ip
cmd:
  netstat -an | findstr 1883
