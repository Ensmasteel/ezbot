#include "stepper/stepper.hpp"

stepper::stepper(){

}

stepper::stepper(int stepPin, int dirPin, int enablePin, int cfg1Pin, int cfg2Pin, int maxSpeed, float acceleration){
    this->_stepPin = stepPin;
    this->_dirPin = dirPin;
    this->_enablePin = enablePin;
    this->_cfg1Pin = cfg1Pin;
    this->_cfg2Pin = cfg2Pin;
    this->_speed = 0;
    this->_targetSpeed = 0;
    this->_position = 0;
    this-> _lastStepTime = 0;

    this->_n = 0;
    this->_c0 = 0.0;
    this->_cn = 0.0;
    this->_cmin = 1.0;


    setMaxSpeed(maxSpeed);
    setAcceleration(acceleration);

    //enable input / outputs
    pinMode(_stepPin, OUTPUT);
    pinMode(_dirPin, OUTPUT);
    pinMode(_enablePin, OUTPUT);
    pinMode(_cfg1Pin, OUTPUT);
    pinMode(_cfg2Pin, OUTPUT);


}


void stepper::setTargetSpeed(float targetSpeed){
    this->_targetSpeed = targetSpeed;
    Serial.print("target speed is now : ");
    Serial.print(targetSpeed);
    Serial.print("\n");
    computeNewSpeed();
}

void stepper::run(){
    //Serial.print("running stepper");
    
    if(!_stepInterval){
        //Serial.print("step interval is 0");
        return;
    }
    unsigned long time = micros();
    

    if (time - _lastStepTime >= _stepInterval) {
        if(_direction == DIRECTION_CW){
            _position++;
        }else{
            _position--;
        }
        step();

        _lastStepTime = time;
        computeNewSpeed();
    }else{
        return;
    }
}


int stepper::getPosition(){
    return _position;
}

float stepper::getSpeed(){
    return _speed;
}

float stepper::getTargetSpeed(){
    return _targetSpeed;
}

void stepper::setMaxSpeed(int speed){
    if (speed < 0.0)
       speed = -speed;
    if (_maxSpeed != speed)
    {
        _maxSpeed = speed;
        _cmin = 1000000.0 / speed;
        // Recompute _n from current speed and adjust speed if accelerating or cruising
        if (_n > 0)
        {
            _n = (long)((_speed * _speed) / (2.0 * _acceleration)); // Equation 16
            computeNewSpeed();
        }
    }
}


void stepper::setAcceleration(float acceleration){

    if(acceleration == 0){
          return;
    }
    if(acceleration < 0){

        acceleration = -acceleration;
    }


    if(_acceleration != acceleration){
        _n = _n * (_acceleration / acceleration);
        _c0 = 0.676 * sqrt(2.0/acceleration) * 1000000.0;
        _acceleration = acceleration;
        computeNewSpeed();
    }
}

int stepper::getMaxSpeed(){
    return _maxSpeed;
}

float stepper::getAcceleration(){
    return _acceleration;
}

unsigned long stepper::computeNewSpeed(){
    
    if (_targetSpeed == 0.0){
        _stepInterval = 0.0;
        _speed = 0.0;
        return _stepInterval;
    }

    _stepInterval = 1000000.0 / abs(_targetSpeed);
    _direction = (_targetSpeed > 0) ?  DIRECTION_CW : DIRECTION_CCW;
    _speed = _targetSpeed;

    
    return _stepInterval;
}

void stepper::step(){
 
    //set the dir pin correctl
    digitalWrite(_dirPin, _direction ? HIGH : LOW);

    //set the step pin high 
    digitalWrite(_stepPin, HIGH);

    delayMicroseconds(_minPulseWidth);
    digitalWrite(_stepPin, LOW);   

}

void stepper::log(){

    Serial.print(">_lastStepTime:");
    Serial.print(_lastStepTime);
    Serial.print("\n");

    Serial.print(">_cn:");
    Serial.print(_cn);
    Serial.print("\n");

    Serial.print(">_stepInterval:");
    Serial.print(_stepInterval);
    Serial.print("\n");

    Serial.print(">_speed:");
    Serial.print(_speed);
    Serial.print("\n");

    Serial.print(">_targetspeed:");
    Serial.print(_targetSpeed);
    Serial.print("\n");
    
    Serial.print(">stepstostop:");
    Serial.print(stepstostop);
    Serial.print("\n");
    
    Serial.print(">_direction:");
    Serial.print(_direction);
    Serial.print("\n");
}