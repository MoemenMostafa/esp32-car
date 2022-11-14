import { Component, ViewChild } from '@angular/core';
import { JoystickEvent, NgxJoystickComponent } from 'ngx-joystick';
import {webSocket, WebSocketSubject} from 'rxjs/webSocket';
import { JoystickManagerOptions, JoystickOutputData } from 'nipplejs';

@Component({
  selector: 'app-home',
  templateUrl: 'home.page.html',
  styleUrls: ['home.page.scss'],
})
export class HomePage {
  myWebSocket: WebSocketSubject<any>;
  isConnected = false;
  speed = [0,0];
  direction = [0,0];
  dutyCycles = [0,0]

  // Duty Cycle constants
  dCHigh = 255; // max duty cycle
  dCLow = 180; // min duty cycle
  dCRange = this.dCHigh - this.dCLow;
  move;

  @ViewChild('staticJoystic') staticJoystick!: NgxJoystickComponent;
  @ViewChild('dynamicJoystick') dynamicJoystick!: NgxJoystickComponent;
  @ViewChild('semiJoystick') semiJoystick!: NgxJoystickComponent;

  staticOptions: JoystickManagerOptions = {
    mode: 'static',
    position: { left: '50%', top: '50%' },
    color: 'blue',
    size: 200,
  };

  // dynamicOptions: JoystickManagerOptions = {
  //   mode: 'dynamic',
  //   color: 'red',
  //   multitouch: true
  // };

  // semiOptions: JoystickManagerOptions = {
  //   mode: 'semi',
  //   catchDistance: 50,
  //   color: 'purple'
  // };

  staticOutputData!: JoystickOutputData;
  semiOutputData!: JoystickOutputData;
  dynamicOutputData!: JoystickOutputData;

  directionStatic!: string;
  interactingStatic!: boolean;


  constructor() {
    this.connect();
  }

  connect() {
    this.myWebSocket = webSocket("ws://192.168.2.184:80/ws");
    this.myWebSocket.subscribe(
      (data) => {
        console.log("data:", data); 
        this.isConnected = true
      },
      (err) => {
        console.log("err:", err); 
        this.isConnected = false
        this.myWebSocket.unsubscribe();
      }
    );
  }

  onStartStatic(event: JoystickEvent) {
    this.interactingStatic = true;
  }

  onEndStatic(event: JoystickEvent) {
    this.interactingStatic = false;
    this.move = null;
    setTimeout(() => {
      this.moveCmd(true);
    }, 100)
  }

  onMoveStatic(event: JoystickEvent) {
    this.staticOutputData = event.data;
    if (!this.move) {
      this.move = setTimeout(() => {
        this.moveCmd();
        this.move = null;
      }, 100) // not to overload the network 
    }
  }

  getDutyCycles(x) {
    // convert the input to a range between high and low (ex. a number between 180 and 255)
    return (Math.abs(x) * this.dCRange / this.dCHigh) + this.dCLow;
  }


  moveCmd(stop = false)  {
    // data is 10 digits as follows
    // 1: MOTOR group => 1 | 2
    // 2: direction(stop | forward | backward) => 0 | 1 | 2

    let degree, halfRadius, speed;
    degree = this.staticOutputData.angle.degree;
    halfRadius = this.staticOptions.size / 2;
    speed = this.staticOutputData.distance * this.dCHigh / halfRadius;

    if (degree > 0 && degree <= 90) {
      this.speed = [speed,(degree * speed * 2 / 90) - speed];
    }
    else if (degree > 90 && degree <= 180) {
      this.speed = [(((degree - 90) * speed * 2 / 90) - speed) * -1,speed];
    } 
    else if (degree > 180 && degree <= 270) {
      this.speed = [-speed,(((degree - 180) * speed * 2 / 90) - speed) * -1];
    } 
    else if (degree > 270 && degree <= 365) {
      this.speed = [((degree - 270) * speed * 2 / 90) - speed,-speed];
    } 

    if (stop)
      this.direction = [0,0]
    else {
      this.direction[0] = this.speed[0] >= 0 ? 1 : 2;
      this.direction[1] = this.speed[1] >= 0 ? 1 : 2;
    }

    this.dutyCycles[0] = Math.floor(this.getDutyCycles(this.speed[0]));
    this.dutyCycles[1] = Math.floor(this.getDutyCycles(this.speed[1]));

    this.myWebSocket.next(parseInt(`1${this.direction[0]}${this.dutyCycles[0]}`));
    this.myWebSocket.next(parseInt(`2${this.direction[1]}${this.dutyCycles[1]}`));
  }

}
