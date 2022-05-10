#include <MyDisp.h>
#include <stdio.h>
#include <stdlib.h>
#include <xgpio.h>
#include <math.h>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <vector>

#include "MtdsDefs.h"
#include "xparameters.h"
#include "xscugic.h"
#include "xtmrctr.h"


// Defines for interrupt IDs
#define INTC_DEVICE_ID XPAR_PS7_SCUGIC_0_DEVICE_ID
#define GPIO_INT_ID XPAR_FABRIC_AXI_GPIO_0_IP2INTC_IRPT_INTR
#define TIMER_INT_ID XPAR_FABRIC_AXI_TIMER_0_INTERRUPT_INTR

// Global variables accessible from any function
XTmrCtr timer;
XGpio input;
MYDISP display;

// Global Interrupt Controller
static XScuGic GIC;

int switch_val = 0;

// This function initalizes the GIC, including vector table setup and CPSR IRQ
// enable
void initIntrSystem(XScuGic* IntcInstancePtr) {
  XScuGic_Config* IntcConfig;
  IntcConfig = XScuGic_LookupConfig(XPAR_PS7_SCUGIC_0_DEVICE_ID);
  XScuGic_CfgInitialize(IntcInstancePtr, IntcConfig,
                        IntcConfig->CpuBaseAddress);
  Xil_ExceptionInit();
  Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
                               (Xil_ExceptionHandler)XScuGic_InterruptHandler,
                               (void*)IntcInstancePtr);
  Xil_ExceptionEnable();
}



void timerInterruptHandler(void* userParam, u8 TmrCtrNumber) {}

void buttonInterruptHandler(void* instancePointer) {
  switch_val = XGpio_DiscreteRead(&input, 1);
  // 1 left
  // 2 right
  // 4 jump

  // add a delay to eliminate double clicking
  // for (int j = 0; j < 3000; j++)
  //   ;

  XGpio_InterruptClear(&input, GPIO_INT_ID);
}

/**********************************************************/
int jjjj = 1;
enum Type { kMario, kTurtle, kPipe,kLifePipe, kPow, KFloor };
class Rect{
  public:
  Rect(){
      x1_ = 0;
      y1_ = 0;
      x2_ = 0;
      y2_ = 0;
  }
  Rect(int x1,int y1,int x2,int y2){
      x1_ = x1;
      y1_ = y1;
      x2_ = x2;
      y2_ = y2;
  }
  void move(int x,int y){
    x1_+=x;
    x2_+=x;
    y1_+=y;
    y2_+=y;
  }
  int bottom(){
    return y1_;
  }
  int top(){
    return y2_;
  }
  int left(){
    return x1_;
  }
  int right(){
    return x2_;
  }
  int x1_,y1_,x2_,y2_;
};
// Coordinate conversion
void convert(int* x1, int* y1) {
  int temp = *x1;
  *x1 = *y1;
  *y1 = temp;
}
void convert(int& x1, int& y1) {
  int temp = x1;
  x1 = y1;
  y1 = temp;
}
void convert(Rect &rect){
  int t = rect.x1_;
  rect.x1_ = rect.y1_;
  rect.y1_ = t;

  int t2 = rect.x2_;
  rect.x2_ = rect.y2_;
  rect.y2_ = t2;
}

static bool IsRectIntersect(int x1, int y1, int x2, int y2, int x3, int y3,
                            int x4, int y4) {
  int h1 = abs(y2 - y1);
  int w1 = abs(x2 - x1);

  int h2 = abs(y4 - y3);
  int w2 = abs(x4 - x3);

  int c1x = (x1 + x2) / 2;
  int c1y = (y1 + y2) / 2;

  int c2x = (x3 + x4) / 2;
  int c2y = (y3 + y4) / 2;

  int px = abs(c2x - c1x);
  int py = abs(c2y - c1y);

  if (px < (w1 + w2) / 2 && py < (h1 + h2) / 2) {
    return true;
  }
  return false;
}

static bool IsRectIntersect(Rect a,Rect b){
  return IsRectIntersect(a.x1_,a.y1_,a.x2_,a.y2_,b.x1_,b.y1_,b.x2_,b.y2_);
}
static int computeArea(int ax1, int ay1, int ax2, int ay2, int bx1, int by1,
                       int bx2, int by2) {
  int area1 = (ax2 - ax1) * (ay2 - ay1), area2 = (bx2 - bx1) * (by2 - by1);
  int overlapWidth = fmin(ax2, bx2) - fmax(ax1, bx1),
      overlapHeight = fmin(ay2, by2) - fmax(ay1, by1);
  int overlapArea = fmax(overlapWidth, 0) * fmax(overlapHeight, 0);
  return overlapArea;
}

static int computeArea(Rect a, Rect b) {
  return computeArea(a.x1_, a.y1_, a.x2_, a.y2_, b.x1_, b.y1_, b.x2_, b.y2_);
}
// 管道类
class Pipe {
 public:
  Pipe(int x1, int y1, int x2, int y2) {
    type_ = kPipe;
    color_ = clrBlue;
    rect_ = Rect(x1,y1,x2,y2);
  }
  void Update() {
    Rect rect = GetRectClone();
    convert(rect);
    display.setForeground(color_);
    display.drawRectangle(true, rect.x1_, rect.y1_, rect.x2_, rect.y2_);
  }
  Rect GetRectClone(){
    return rect_;
  }
  Rect* GetRect(){
    return &rect_;
  }

 private:
  Type type_;
  uint32_t color_;
  Rect rect_;
};
class Coin {
 public:
  Coin(int x1,int y1,int x2,int y2) {
    type_ = kTurtle;
    state_ = kDeath;
    rect_ = Rect(x1,y1,x2,y2);
    // -1 left 1 right
    direction_ = 1;
    // pink FFC0CB
    color_ = clrDkMagenta;
    x_vel = 0;
    y_vel =0;
  }
  void Update() {
    if (state_ == kDeath) {
      rect_.x1_ = 0;
      rect_.x2_ = 0;
      rect_.y1_ = 0;
      rect_.y2_ = 0;
      return;
    }
    HandleState();
    Rect rect = GetRectClone();
    convert(rect);
    display.setForeground(color_);
    display.drawEllipse(true, rect.x1_, rect.y1_, rect.x2_, rect.y2_);
  }
  void HandleState() {
    if (state_ == kDeath) {
      return;
    }
    if (state_ == kWalk) {
      // 向左
      if (direction_ == -1) {
        x_vel = -8;
        y_vel =0;
        //向右
      } else if (direction_ == 1) {
        x_vel = 8;
        y_vel = 0;
      }
    } else if (state_ == kFall) {
      x_vel = 0;
      y_vel = -10;
    }
  }
  void Kill(){
    SetState(kDeath);
  }

  Rect GetRectClone(){
    return rect_;
  }
  Rect* GetRect(){
    return &rect_;
  }

  enum State { kWalk,kFall,kDeath};
  void SetState(State state) { state_ = state; };
  State GetState(){return state_;};
  int x_vel;
  int y_vel;
  int direction_;
  uint32_t color_;
 private:
  State state_;
  Type type_;
  Rect rect_;
};
class LifePipe {
 public:
  LifePipe(int x1,int y1,int x2,int y2) {
    type_ = kLifePipe;
    color_ = clrBlue;
    // rect_ = Rect(150, 195, 170, 205);
    rect_ = Rect(x1, y1, x2, y2);
    state_ = kDis;
  }
  void Update() {
    if (state_ == kDeath || state_ == kDis) {
      rect_.x1_ = 0;
      rect_.x2_ = 0;
      rect_.y1_ = 0;
      rect_.y2_ = 0;
      return;
    }
    if (state_ == kReady) {
      Rect rect = GetRectClone();
      convert(rect);
      display.setForeground(color_);
      display.drawRectangle(true, rect.x1_, rect.y1_, rect.x2_, rect.y2_);
    }
  }

  Rect GetRectClone(){
    return rect_;
  }
  Rect* GetRect(){
    return &rect_;
  }
  enum State {kDis,kReady,kDeath};
  State GetState(){return state_;};
  void SetState(State state){state_ = state;};
 private:
  Type type_;
  uint32_t color_;
  Rect rect_;
  State state_;
};
class Pow {
 public:
  Pow() {
    state_ = kReady;
    type_ = kPow;
    color_ = clrDkBlue;
    times = 3;
    int x1_ = 145;
    int y1_ = 70;
    int x2_ = 145 + 30;
    int y2_ = 70 + 10;
    rect_ = Rect(x1_,y1_,x2_,y2_);
  }
  void Update() {
    if(state_ == kDeath){
      return;
    }
    HandleState();
    Rect rect = GetRectClone();
    convert(rect);
    display.setForeground(color_);
    display.drawRectangle(true, rect.x1_, rect.y1_, rect.x2_, rect.y2_);
    
    display.setForeground(clrWhite);
    char L[2];
    memset(L,0,2);
    snprintf(L, 2, "%d",times);
    display.drawText(L,72,155);
  }
  void HandleState(){
    if(state_ == kHit){
      times-=1;
      if(times <=0){
        SetState(kDeath);
      }else{
      SetState(kReady);
      }
    }
  }
  Rect GetRectClone(){
    return rect_;
  }
  Rect* GetRect(){
    return &rect_;
  }
  enum State {kReady,kHit,kDeath};
  State GetState(){return state_;};
  void SetState(State state){state_ = state;};
 private:
  // int x1_, y1_, x2_, y2_;
  State state_;
  int times;
  Type type_;
  uint32_t color_;
  Rect rect_;
};

class Floor {
 public:
  Floor() {
    type_ = KFloor;
    color_ = clrRed;
    int x1_ = 0;
    int y1_ = 0;
    int x2_ = 320;
    int y2_ = 30;
    rect_ = Rect(x1_,y1_,x2_,y2_);
  }
  void Update() {
    Rect rect = GetRectClone();
    convert(rect);
    display.setForeground(color_);
    display.drawRectangle(true, rect.x1_, rect.y1_, rect.x2_, rect.y2_);
  }
  Rect GetRectClone(){
    return rect_;
  }
  Rect* GetRect(){
    return &rect_;
  }

 private:
  Type type_;
  uint32_t color_;
  Rect rect_;
};

class Turtle {
 public:
  Turtle(int x1,int y1,int x2,int y2) {
    type_ = kTurtle;
    state_ = kWalk;
    rect_ = Rect(x1,y1,x2,y2);
    // -1 left 1 right
    direction_ = 1;
    // pink FFC0CB
    color_ = clrDkRed;
    x_vel = 0;
    y_vel =0;
  }
  void Update() {
    if(state_ == kDeath){
      return;
    }
    HandleState();
    Rect rect = GetRectClone();
    convert(rect);
    display.setForeground(color_);
    display.drawRectangle(true, rect.x1_, rect.y1_, rect.x2_, rect.y2_);
  }
  void HandleState() {
    if (state_ == kDeath) {
      return;
    }
    else if (state_ == kWalk) {
      // 向左
      if (direction_ == -1) {
        x_vel = -5;
        //向右
      } else if (direction_ == 1) {
        x_vel = 5;
      }
    } else if (state_ == kVertigo) {
      x_vel = 0;
      y_vel = 0;
    } else if (state_ == kFall) {
      x_vel = 0;
      y_vel = -10;
    }
  }
  void Kill(){
    SetState(kDeath);
  }

  Rect GetRectClone(){
    return rect_;
  }
  Rect* GetRect(){
    return &rect_;
  }

  enum State { kWalk,kFall,kVertigo,kDeath};
  void SetState(State state) { state_ = state; };
  State GetState(){return state_;};
  int x_vel;
  int y_vel;
  int direction_;
  uint32_t color_;
 private:
  State state_;
  Type type_;
  Rect rect_;
};

class Mario {
 public:
  Mario() {
    type_ = kMario;
    state_ = kJump;
    color_ = clrYellow;
    int x1_ = 50+0;
    int y1_ = 30;
    int x2_ = 60+0;
    int y2_ = 50;
    rect_ = Rect(x1_, y1_, x2_, y2_);
    x_vel = 0;
    y_vel = 0;
  }
  void Update() {
    if(state_ == kDeath){
      return;
    }
    HandleState();
    Rect rect = GetRectClone();
    convert(rect);
    display.setForeground(color_);
    display.drawRectangle(true, rect.x1_, rect.y1_, rect.x2_, rect.y2_);
  }
  void HandleState() {
    if (state_ == kStanding) {
      if (switch_val == 1) {
        x_vel = -5;
        SetState(kWalk);
      } else if (switch_val == 2) {
        x_vel = 5;
        SetState(kWalk);
      } else if (switch_val == 4) {
        SetState(kJump);
        // y_acc = 0;
      } else {
        SetState(kStanding);
      }
    } else if (state_ == kWalk) {
      // 1 left
      // 2 right
      // 4 jump
      if (switch_val == 1) {
        x_vel = -5;
        SetState(kStanding);
      } else if (switch_val == 2) {
        x_vel = 5;
        SetState(kStanding);
      } else {
        x_vel = 0;
        SetState(kStanding);
      }
    } else if (state_ == kJump) {
      y_vel = 20;
      x_vel = 0;
      SetState(kFall);
    } else if (state_ == kFall) {
      x_vel = 0;
      y_vel = -5;
    }else if(state_ == kDeath){

    }
  }
  Rect GetRectClone(){
    return rect_;
  }
  Rect* GetRect(){
    return &rect_;
  }
  enum State { kStanding,kWalk,kJump,kFall,kDeath};
  void SetState(State state) { state_ = state; };
  State GetState(){return state_;};
  int x_vel;
  int y_vel;
  int y_acc;
 private:
  Type type_;
  State state_;
  uint32_t color_;
  Rect rect_;
};

class GameLoop {
 public:
  void Init() {
    Floor* f = new Floor();
    floor_.push_back(f);

    int pipe_arr[7][4] = {{0, 170, 140, 180},   {180, 170, 320, 180},

                          {0, 120, 50, 130},    {85, 120, 235, 130},
                          {270, 120, 320, 130},

                          {0, 70, 115, 80},     {205, 70, 320, 80}};
    for (int i = 0; i < 7; i++) {
      Pipe* p = new Pipe(pipe_arr[i][0], pipe_arr[i][1], pipe_arr[i][2],
                         pipe_arr[i][3]);
      pipe_.push_back(p);
    }
    int turtle_arr[][4] = {{20, 180, 30, 190}, {200, 180, 210, 190},
    {40,80,50,90}
    };
    for (int j = 0; j < 3; j++) {
      Turtle* t = new Turtle(turtle_arr[j][0], turtle_arr[j][1],
                             turtle_arr[j][2], turtle_arr[j][3]);
      turtle_.push_back(t);
      t->color_ = ChooseColor(j*j);
    }
    turtle_[0]->direction_ = 1;
    turtle_[1]->direction_ = -1;

    // turtle_[0]->color_ = clrRed;
    // turtle_[1]->color_ = clrDkCyan;

    Pow *pow = new Pow();
    pow_.push_back(pow);

    mario_ = new Mario();
    lifepipe_ = new LifePipe(0,0,0,0);
    coin_ = new Coin(0,0,0,0);
    life_ = 3;
    score_ = 0;
    state_ = kGame;
    iter_ = 0;
  }
  int ChooseColor(int i){
    if(i>13){
      i = 0;
    }
    int co[] = {clrRed,        clrDkRed,      clrGreen,   clrDkGreen,
                clrBlue,       clrDkBlue,     clrYellow,  clrDkYellow,
                clrCyan,       clrDkCyan,     clrMagenta, clrDkMagenta,
                clrLtBlueGray, clrMedBlueGray};

    return co[i];
  }
  void DrawLife(){
    int life_arr[3][4] = {
        {0, 220, 15, 235}, {25, 220, 40, 235}, {50, 220, 65, 235}};
    for (int i = 0; i < life_; i++) {
      display.setForeground(clrRed);
      // display.drawRectangle(bool fill, int x1, int y1, int x2, int y2)
      display.drawEllipse(true, life_arr[i][1], life_arr[i][0],
                            life_arr[i][3], life_arr[i][2]);
    }
  }
  void DrawScore(){
    // 更新分数
    char score[16];
    memset(score,0,16);
    snprintf(score, 16, "S:%d",score_);
    int x = 300;
    int y = 190;
    convert(x,y);
    display.setForeground(clrWhite);
    display.drawText(score, x,y);
  }
  void Loop() {
    while (true) {
      ++iter_;
      display.clearDisplay(clrBlack);
      if (state_ == kGame) {
        LoopStart();
      } else if (state_ == kPause) {
        char buffer[16];
        memset(buffer, 0, 16);
        snprintf(buffer, 16, "Game Pause!");
        int x = 150;
        int y = 140;
        convert(x, y);
        display.setForeground(clrWhite);
        display.drawText(buffer, x, y);
      } else if (state_ == kGameOver) {
        char buffer[16];
        memset(buffer, 0, 16);
        snprintf(buffer, 16, "Game Over!");
        int x = 150;
        int y = 140;
        convert(x, y);
        display.setForeground(clrWhite);
        display.drawText(buffer, x, y);
        break;
      }
      delay(166);
    }
  }
  void LoopStart(){
  //  display.clearDisplay(clrBlack);
  if(life_ <=0){
    SetState(kGameOver);
    return;
  }
  if (switch_val == 8) {
    if (state_ == kGame) {
      SetState(kPause);
      switch_val =0;
    } else if (state_ == kPause) {
      SetState(kGame);
      switch_val =0;
    }
  }
   UpdateAll();
  }
  void UpdateAll(){
    mario_->Update();
    for (auto p:pow_){
      p->Update();
    }
    for (auto f : pipe_) {
      f->Update();
    }
    for (auto f : floor_) {
      f->Update();
    }
    for (auto f : turtle_) {
      f->Update();
    }
    lifepipe_->Update();
    coin_->Update();

    DrawScore();
    DrawLife();
    // 这里才真正的改变物体的位置
    AdjustMarioPosition();
    AdjustTurtlePosition();
    // 随机出现coin
    if(iter_ % 50 == 0 && coin_->GetState()==Coin::kDeath){
      Rect *ci = coin_->GetRect();
      ci->x1_ = 0;
      ci->y1_ = 180;
      ci->x2_ = 10;
      ci->y2_ = 190;
      coin_->SetState(Coin::kWalk);
      coin_->direction_ = 1;
    }
    else if (iter_ % 80 == 0 && coin_->GetState() == Coin::kDeath) {
      Rect* ci = coin_->GetRect();
      ci->x1_ = 300;
      ci->y1_ = 180;
      ci->x2_ = 310;
      ci->y2_ = 190;
      coin_->SetState(Coin::kWalk);
      coin_->direction_ = -1;
    }
    AdjustCoinPosition();
    CheckLifePipeAlive();
    
  }
  void CheckLifePipeAlive(){
    if(lifepipe_->GetState() == LifePipe::kReady){
      Rect c_life = lifepipe_->GetRectClone();
      Rect mar = mario_->GetRectClone();
      mar.move(0, -3);
      if (!IsRectIntersect(mar, c_life)) {
          lifepipe_->SetState(LifePipe::kDeath);
      }
    }

  }
  void AdjustCoinPosition(){
      if(coin_->GetState() == Coin::kDeath){
        return;
      }
      Rect* rect = coin_->GetRect();
      rect->move(coin_->x_vel,coin_->y_vel);
      if (rect->x2_ <= 0) {
        rect->x1_ = 320 - 10;
        rect->x2_ = 320;
      }
      if (rect->x1_ >= 320) {
        rect->x1_ = 0;
        rect->x2_ = 10;
      }
      if (rect->right() <= 5 && rect->top() < 50) {
        coin_->SetState(Coin::kDeath);
      }
      if (rect->left() >= 315 && rect->top() < 50) {
        coin_->SetState(Coin::kDeath);
      }
      // xxxxxxxxxxx
      Rect rect_t = coin_->GetRectClone();
      // 是否碰到了其它乌龟,如果碰到了的话 那么就改变他们的方向
      for (auto tur : turtle_) {
        Rect current = tur->GetRectClone();
        if (tur->GetRect()->bottom() != coin_->GetRect()->bottom()) {
            continue;
        }
        if (IsRectIntersect(rect_t, current)) {
          coin_->direction_ = -coin_->direction_;
          tur->direction_ = -tur->direction_;
          // 重新调整位置
          // 现在在被碰撞的左边
          // 直接向各自的反方向运行
          coin_->GetRect()->move(5 * coin_->direction_, 0);
          tur->GetRect()->move(5 * tur->direction_, 0);
        }
      }
      // yyyyyyyyyy
      Rect coin = coin_->GetRectClone();
      coin.move(0, -1);
      Floor* is_floor = nullptr;
      Pipe* is_pipe = nullptr;
      for (auto f : floor_) {
        Rect f_rect = f->GetRectClone();
        if (IsRectIntersect(coin, f_rect)) {
          is_floor = f;
        }
      }
      for (auto p : pipe_) {
        Rect current = p->GetRectClone();
        // 如果碰到了水管
        if (IsRectIntersect(coin, current)) {
          is_pipe = p;
          break;
        }
      }
      if (is_floor == nullptr && is_pipe == nullptr) {
        coin_->SetState(Coin::kFall);
      }else{
        coin_->SetState(Coin::kWalk);
      }
      coin.move(0, 1);
  }
  void AdjustMarioPosition(){
     // 这时候物体的状态已经被改变了，现在物体的位置就在最终的位置上
     // 但是物体的状态还是要下一次才会被绘制
    Rect* rect = mario_->GetRect();
    rect->move(mario_->x_vel,mario_->y_vel);
    if (rect->x2_ <= 0) {
      rect->x1_ = 320 - 10;
      rect->x2_ = 320;
    }
    if (rect->x1_ >= 320) {
      rect->x1_ = 0;
      rect->x2_ = 10;
    }
    // 检查mario的x是否有碰撞
    CheckMarioXCollisions();

    // 检查mario的y是否有碰撞
    CheckMarioYCollisions();
  }
  void AdjustTurtlePosition(){
    for (auto t : turtle_) {
      if (t->GetState() == Turtle::kDeath) {
        continue;
      }
      Rect* rect = t->GetRect();
      rect->move(t->x_vel,t->y_vel);

      if (rect->right() <= 0 && rect->bottom() > 30) {
        rect->x1_ = 320 - 10;
        rect->x2_ = 320;
      }
      if (rect->left() >= 320 && rect->bottom() > 30) {
        rect->x1_ = 0;
        rect->x2_ = 10;
      }
      CheckTurtleXCollisions(t);
      CheckTurtleYCollisions(t);

    }
  }
  void CheckTurtleXCollisions(Turtle* t) {
    if(t->GetState() != Turtle::kWalk){
      return;
    }
    Rect rect_t = t->GetRectClone();
    // 是否碰到了其它乌龟,如果碰到了的话 那么就改变他们的方向
    for (auto tur : turtle_) {
      if (tur == t) {
        continue;
      }
      Rect current = tur->GetRectClone();
      if(tur->GetRect()->bottom() != t->GetRect()->bottom()){
        return;
      }
      if (IsRectIntersect(rect_t, current)) {
        t->direction_ = -t->direction_;
        tur->direction_ = -tur->direction_;
        // 重新调整位置
        // 现在在被碰撞的左边
        // 直接向各自的反方向运行
        t->GetRect()->move(5*t->direction_,0);
        tur->GetRect()->move(5*tur->direction_,0);
      }
    }
  }
  void CheckTurtleYCollisions(Turtle* t) {
    // 是否一直在地板上，不然的话就下降
    Rect tur = t->GetRectClone();
    tur.move(0, -5);
    Floor* is_floor = nullptr;
    Pipe* is_pipe = nullptr;
    for (auto f : floor_) {
      Rect f_rect = f->GetRectClone();
      if (IsRectIntersect(tur, f_rect)) {
        is_floor = f;
      }
    }
    for (auto p : pipe_) {
      Rect current = p->GetRectClone();
      // 如果碰到了水管
      if (IsRectIntersect(tur, current)) {
        is_pipe = p;
        break;
      }
    }
    if(is_floor == nullptr && is_pipe == nullptr){
      t->SetState(Turtle::kFall);
    }else{
      // 如果有一个碰到了的话，就不再下落
      if (t->GetState() != Turtle::kVertigo) {
        t->y_vel = 0;
        t->SetState(Turtle::kWalk);
        // 如果是在最下层 并且到了边缘，
        // 那么就让它从最上层的另一边出来
        if (is_floor) {
          Rect* p_rect = t->GetRect();
          if (p_rect->right() <= 10) {
            p_rect->x1_ = 320 - 10;
            p_rect->x2_ = 320;
            p_rect->y1_ = 180;
            p_rect->y2_ = 190;
          }
          else if (p_rect->left() >= 310) {
            p_rect->x1_ = 0;
            p_rect->x2_ = 10;
            p_rect->y1_ = 180;
            p_rect->y2_ = 190;
          }
        }
      }
    }
    tur.move(0, 5);
  }
  void CheckMarioXCollisions(){
      // 是否撞到了乌龟
      Turtle *t = nullptr;
      Coin *c = nullptr;
      for (auto turtle : turtle_) {
        if (turtle->GetState() == Turtle::kDeath) {
          continue;
        }
        Rect tur = turtle->GetRectClone();
        Rect mar = mario_->GetRectClone();
        if (IsRectIntersect(mar, tur)) {
          t = turtle;
          break;
        }
      }
      //是否撞到了金币
      Rect cion = coin_->GetRectClone();
      Rect mar = mario_->GetRectClone();
      if (IsRectIntersect(mar, cion)) {
        c = coin_;
      }
      if(c){
        score_+=800;
        c->SetState(Coin::kDeath);
      }
      // 如果有撞到的话
      if (t) {
        if (t->GetState() == Turtle::kVertigo) {
          // 并且加分
          t->Kill();
          score_ += 800;
        } else {
          // 死亡 并且从上面复活
          // Rect(150, 195, 170, 205);
          // mario_->SetState(Mario::kDeath);
          Rect * r = mario_->GetRect();
          Rect * life = lifepipe_->GetRect();

          r->x1_ = 155;
          r->x2_ = 165;
          r->y1_ = 205;
          r->y2_ = 225;
          life_ -=1;
          mario_->SetState(Mario::kStanding);
          lifepipe_->SetState(LifePipe::kReady);
          // Rect(150, 195, 170, 205);
          life->x1_ = 150;
          life->y1_ = 195;
          life->x2_ = 170;
          life->y2_ = 205;
        }
      }
  }
  void CheckMarioYCollisions() {
    // 可能的状态有kJump,kFall,kStand,kWalk
    Rect mar = mario_->GetRectClone();
    // 现在我们只跳到了管子的下面
    // 我们假设会和管子重合
    mar.move(0,5);

    Mario::State current_state = mario_->GetState();
    // 是否撞到了pow
    Pow* is_pow = nullptr;
    Pipe* is_pipe = nullptr;
    Floor* is_floor = nullptr;

    // 是否撞到了pow
    for (auto pp : pow_) {
      if (pp->GetState() == Pow::kDeath) {
        continue;
      }
      Rect current = pp->GetRectClone();
      if (IsRectIntersect(mar, current)) {
        is_pow = pp;
        break;
      }
    }
    // 是否撞到了水管
    for (auto p : pipe_) {
      Rect current = p->GetRectClone();
      // 如果碰到了水管
      if (IsRectIntersect(mar, current)) {
        is_pipe = p;
        break;
      }
    }

    // 是否撞到了地板
    for (auto f : floor_) {
      Rect current = f->GetRectClone();
      if (IsRectIntersect(mar, current)) {
        is_floor = f;
        break;
      }
    }
    // 游戏逻辑
    if (is_pow) {
      // 如果是跳的时候撞到了pow，那么就执行pow的功能
      // if (current_state == Mario::kJump) {
        is_pow->SetState(Pow::kHit);
        // 所有乌龟变晕,就是反转状态
        for (auto t : turtle_) {
          if (t->GetState() == Turtle::kDeath) {
            continue;
          }
          if (t->GetState() == Turtle::kWalk) {
            t->SetState(Turtle::kVertigo);
          } else if (t->GetState() == Turtle::kVertigo) {
            t->SetState(Turtle::kWalk);
          }
        }
    } else if (is_floor) {
      //让mario站在floor上,必然会碰到floor
      // 修正mario的位置
      Rect *t_mar = mario_->GetRect();
      Rect *t_fl = is_floor->GetRect();
      // 下降
      if(mar.top() > t_fl->top()){
          t_mar->y1_ = t_fl->y2_;
          t_mar->y2_ = t_mar->y1_ + 20;
          mario_->SetState(Mario::kStanding);
          mario_->y_vel = 0;
      }
    } else if (is_pipe) {
      Rect *t_mar = mario_->GetRect();
      Rect *t_pipe = is_pipe->GetRect();
      // 下降
      if(mar.top() > is_pipe->GetRect()->top()){
          t_mar->y1_ = t_pipe->y2_;
          t_mar->y2_ = t_mar->y1_ + 20;
          mario_->SetState(Mario::kStanding);
          mario_->y_vel = 0;
      }
      //上去  
      else if(mar.top() < is_pipe->GetRect()->top()){
        //判断是否有乌龟
        mar.move(0, 15);
        for (auto turtle : turtle_) {
          if (turtle->GetState() == Turtle::kDeath) {
            continue;
          }
          Rect tur = turtle->GetRectClone();
          if (IsRectIntersect(tur, mar)) {
            // 如果有，就把乌龟置为晕的状态，乌龟变为粉色
            // 否则就反转乌龟的状态
            if (turtle->GetState() == Turtle::kVertigo) {
              turtle->SetState(Turtle::kWalk);
            } else {
              turtle->SetState(Turtle::kVertigo);
            }
          }
        }
         //传送到上面去
          // 如果交集少，那么就传送上去
        if(computeArea(mar,*t_pipe) < 80){
            t_mar->move(0,30);
            mario_->y_vel = 0;
            mario_->SetState(Mario::kStanding);
        }
        mar.move(0,-15);
      }else {
       
      }
    } 
    mar.move(0,-5);
    TestMarioIsFalling();
  }
  void TestMarioIsFalling(){

    Rect mar = mario_->GetRectClone();
    Mario::State current_state = mario_->GetState();
    
    mar.move(0,-3);
    // 是否撞到了pow
    Pow* is_pow = nullptr;
    Pipe* is_pipe = nullptr;
    Floor* is_floor = nullptr;
    LifePipe *is_lifepipe = nullptr;

    Rect c_life = lifepipe_->GetRectClone();
    if (IsRectIntersect(mar, c_life)) {
      is_lifepipe = lifepipe_;
    }
    // 是否撞到了pow
    for (auto pp : pow_) {
      if (pp->GetState() == Pow::kDeath) {
        continue;
      }
      Rect current = pp->GetRectClone();
      if (IsRectIntersect(mar, current)) {
        is_pow = pp;
        break;
      }
    }
    // 是否撞到了水管
    for (auto p : pipe_) {
      Rect current = p->GetRectClone();
      // 如果碰到了水管
      if (IsRectIntersect(mar, current)) {
        is_pipe = p;
        break;
      }
    }

    // 是否撞到了地板
    for (auto f : floor_) {
      Rect current = f->GetRectClone();
      if (IsRectIntersect(mar, current)) {
        is_floor = f;
        break;
      }
    }
    if (is_floor == nullptr && is_pipe == nullptr && is_pow == nullptr &&
        current_state != Mario::kJump && is_lifepipe == nullptr) {
      // 让mario下落
      mario_->SetState(Mario::kFall);
    }
    mar.move(0,3);
  }
  void delay(int ms) {
    int one_ms = 33333333 / 1000;
    for (int j = 0; j < ms * one_ms; ++j) {
    }
  }
  void clear(Type type,void *obj){
    if(type == kPow){
        pow_.pop_back();
    }
  }
  enum State {kGame,kPause,kGameOver};
  State GetState(){return state_;};
  void SetState(State state){state_ = state;};
 private:
  std::vector<Floor*> floor_;
  std::vector<Pow*> pow_;
  std::vector<Pipe*> pipe_;
  std::vector<Turtle*> turtle_;
  Mario* mario_;
  LifePipe *lifepipe_;
  Coin *coin_;
  State state_;
  int life_;
  int score_;
  int iter_;
};
/**********************************************************/
int main() {
  initIntrSystem(&GIC);

  // Configure GPIO input and output and set direction
  // Note that the input variable is already declared as a global variable
  XGpio_Initialize(&input, XPAR_AXI_GPIO_0_DEVICE_ID);
  XGpio_SetDataDirection(&input, 1, 0xF);  // 1 = input, 0 = output

  // Configure Timer and timer interrupt
  XTmrCtr_Initialize(&timer, XPAR_AXI_TIMER_0_DEVICE_ID);
  XTmrCtr_SetHandler(&timer, (XTmrCtr_Handler)timerInterruptHandler,
                     (void*)0x12345678);
  XScuGic_Connect(&GIC, TIMER_INT_ID,
                  (Xil_InterruptHandler)XTmrCtr_InterruptHandler, &timer);
  XScuGic_Enable(&GIC, TIMER_INT_ID);
  XScuGic_SetPriorityTriggerType(&GIC, TIMER_INT_ID, 0x0, 0x3);
  XTmrCtr_SetOptions(&timer, 0, XTC_INT_MODE_OPTION | XTC_AUTO_RELOAD_OPTION);
  XTmrCtr_SetResetValue(&timer, 0, 0xFFFFFFFF - 33333333);  // 1 Hz
  XTmrCtr_Start(&timer, 0);

  // Configure GPIO interrupt
  XScuGic_Connect(&GIC, GPIO_INT_ID,
                  (Xil_ExceptionHandler)buttonInterruptHandler, &input);
  XGpio_InterruptEnable(&input, XGPIO_IR_CH1_MASK);
  XGpio_InterruptGlobalEnable(&input);
  XScuGic_Enable(&GIC, GPIO_INT_ID);
  XScuGic_SetPriorityTriggerType(&GIC, GPIO_INT_ID, 0x8, 0x3);

  display.begin();
  display.clearDisplay(clrBlack);
  GameLoop loop;
  loop.Init();

  loop.Loop();

}
