#include <MyDisp.h>
#include <stdio.h>
#include <stdlib.h>
#include <xgpio.h>

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
  for (int j = 0; j < 3000; j++)
    ;

  XGpio_InterruptClear(&input, GPIO_INT_ID);
}

/**********************************************************/

enum Type { kMario, kTurtle, kPipe, kPow, KFloor };
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
    Rect rect = GetRectClone();
    convert(rect);
    display.setForeground(color_);
    display.drawRectangle(true, rect.x1_, rect.y1_, rect.x2_, rect.y2_);
  }
  void HandleState(){
    if(state_ == kHit){
      times-=1;
      if(times <=0){
        SetState(kDeath);
      }
      SetState(kReady);
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
    if (state_ == kWalk) {
      // 向左
      if (direction_ == -1) {
        x_vel = -5;
        //向右
      } else if (direction_ == 1) {
        x_vel = 5;
      }
    } else if (state_ == kVertigo) {
      x_vel = 0;
    } else if (state_ == kFall) {
      y_vel = -50;
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
    state_ = kWalk;
    color_ = clrYellow;
    int x1_ = 50;
    int y1_ = 30;
    int x2_ = 60;
    int y2_ = 50;
    rect_ = Rect(x1_, y1_, x2_, y2_);
    g = 1.01;
    x_vel = 0;
    y_vel = 0;
    y_acc = 0;
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
        SetState(kWalk);
      } else if (switch_val == 2) {
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
      } else if (switch_val == 2) {
        x_vel = 5;
      } else {
        x_vel = 0;
        SetState(kStanding);
      }
    } else if (state_ == kJump) {
      if(y_acc < 45){
        y_vel = 5;
        y_acc+=5;
      }else{
      SetState(kFall);
      y_acc = 0;
      }
    } else if (state_ == kFall) {
      if(y_acc < 55){
        y_acc += 5;
        y_vel = -5;
      }else {
      // SetState(kStanding);
      y_acc = 0;
      }
      // y_vel = -55;
    }else if (state_ == kDeath) {
      
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
  float g;
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
    int turtle_arr[2][4] = {{20, 180, 40, 190}, {200, 180, 220, 190}};
    for (int j = 0; j < 2; j++) {
      Turtle* t = new Turtle(turtle_arr[j][0], turtle_arr[j][1],
                             turtle_arr[j][2], turtle_arr[j][3]);
      turtle_.push_back(t);
    }
    turtle_[0]->direction_ = 1;
    turtle_[1]->direction_ = -1;
    turtle_[1]->color_ = clrDkCyan;

    Pow *pow = new Pow();
    pow_.push_back(pow);

    mario_ = new Mario();

    life_ = 3;
    score_ = 0;
  }
  void Loop() {
    while (true) {
      LoopStart();
      delay(200);
    }
  }
  void LoopStart(){
   display.clearDisplay(clrBlack);
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
    // 更新分数和生命
    char score[50];
    memset(score,0,50);
    snprintf(score, 50, "Your Score: %d",score_);
    int x = 160;
    int y = 230;
    convert(x,y);
    display.drawText(score,x,y);
   
    char life[50];
    memset(life,0,50);
    snprintf(life, 50, "Your Life: %d",life_);
    int x1 = 160;
    int y1 = 210;
    convert(x,y);
    display.drawText(life,x1,y1);

    // 这里才真正的改变物体的位置
    AdjustMarioPosition();
    AdjustTurtlePosition();
  }
  void AdjustMarioPosition(){
     // 这时候物体的状态已经被改变了，现在物体的位置就在最终的位置上
     // 但是物体的状态还是要下一次才会被绘制
    Rect* rect = mario_->GetRect();
    // rect->x1_ += mario_->x_vel;
    // rect->x2_ += mario_->x_vel;
    // rect->y1_ += mario_->y_vel;
    // rect->y2_ += mario_->y_vel;
    rect->move(mario_->x_vel,mario_->x_vel);
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
      // rect->x1_ += t->x_vel;
      // rect->x2_ += t->x_vel;
      // rect->y1_ += t->y_vel;
      // rect->y2_ += t->y_vel;
      rect->move(t->x_vel,t->y_vel);


      if (rect->right() <= 0 && rect->bottom() > 30) {
        rect->x1_ = 320 - 20;
        rect->x2_ = 320;
      }
      if (rect->left() >= 320 && rect->bottom() > 30) {
        rect->x1_ = 0;
        rect->x2_ = 20;
      }
      CheckTurtleXCollisions(t);
      CheckTurtleYCollisions(t);

    }
  }
  void CheckTurtleXCollisions(Turtle* t) {
    Rect rect_t = t->GetRectClone();
    // 是否碰到了其它乌龟,如果碰到了的话 那么就改变他们的方向
    for (auto tur : turtle_) {
      if (tur == t) {
        continue;
      }
      Rect current = tur->GetRectClone();
      if (IsRectIntersect(rect_t, current)) {
        t->direction_ = -t->direction_;
        tur->direction_ = -tur->direction_;
        // 重新调整位置
        // 现在在被碰撞的左边
        if (t->GetRect()->left() < current.left()) {
          // t->GetRect()->move(-7,0);
          t->GetRect()->x2_ = current.left();
          t->GetRect()->x1_ = t->GetRect()->x2_ - 20;
        } else {
          // t->GetRect()->move(7,0);
          t->GetRect()->x1_ = current.right();
          t->GetRect()->x2_ = t->GetRect()->x1_ + 20;
        }
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
      t->y_vel = 0;
      t->SetState(Turtle::kWalk);
      // 如果是在最下层 并且到了边缘，
      // 那么就让它从最上层的另一边出来
      if(is_floor){
        Rect *p_rect = t->GetRect();
        if (p_rect->right() <= 5) {
          p_rect->x1_ = 320 - 20;
          p_rect->x2_ = 320;
          p_rect ->y1_ = 180;
          p_rect->y2_ = 190;
        }
        if (p_rect->left() >= 315) {
          p_rect->x1_ = 0;
          p_rect->x2_ = 20;
          p_rect->y1_ = 180;
          p_rect->y2_ = 190;
        }
      }
    }
    tur.move(0, 5);

  }
  void CheckMarioXCollisions(){
      // 是否撞到了乌龟
      Turtle *t = nullptr;
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
      // 如果有撞到的话
      if (t) {
        if (t->GetState() == Turtle::kVertigo) {
          // 并且加分
          t->Kill();
          score_ += 800;
        } else {
          mario_->SetState(Mario::kDeath);
          life_ -=1;
        }
      }
  }
  void CheckMarioYCollisions() {
    // 可能的状态有kJump,kFall,kStand,kWalk
    Rect mar = mario_->GetRectClone();
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
      if (current_state == Mario::kJump) {
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
        mario_->SetState(Mario::kFall);

      }else if(current_state == Mario::kFall){
        // 那么让就mario站在pow上
        // TODO 
      }
    } else if (is_floor) {
      //让mario站在floor上,必然会碰到floor
      // 修正mario的位置
      if(current_state == Mario::kFall){
          Rect *t_fl = is_floor->GetRect();
          Rect *t_mar = is_floor->GetRect();

          t_mar->y1_ = t_fl->y2_;
          t_mar->y2_ = t_fl->y1_ + 20;

          mario_->y_vel = 0;
          mario_->SetState(Mario::kStanding);
      }
    } else if (is_pipe) {
      // 如果是下落的话 如果是落在了管子上面
      if (mario_->GetState() == Mario::kFall) {
      // if (mar.bottom() > is_pipe->GetRect()->bottom()) {
        //修正mario的位置
        Rect* t_mar = mario_->GetRect();

        // 还原位置
        t_mar->move(-mario_->x_vel, -mario_->y_vel);
        // 到下一层的梯子上
        t_mar->y1_ = is_pipe->GetRect()->y2_;
        t_mar->y2_ = t_mar->y1_ + 20;
        mario_->SetState(Mario::kStanding);

        // t_mar->move(50, 50);
      } else if (mario_->GetState() == Mario::kJump) {
      // } else if (mar.top() < is_pipe->GetRect()->top()) {
        // 如果是跳起来的话
        //判断水管上面有没有乌龟
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
        // 修正mario的位置
        Rect* rect = mario_->GetRect();
        if(mario_->y_acc == 40){
        // 复原原来的位置
        rect->move(-mario_->x_vel, -mario_->y_acc);
        // rect->y1_ -= mario_->y_vel;
        // rect->y2_ -= mario_->y_vel;
        // rect->y1_ += 20;
        // rect->y2_ += 20;
        // 刚好顶着水管
        rect->move(0, 20);
        // mario_->SetState(Mario::kFall);
        }

      }
    } 
    TestMarioIsFalling();
  }
  void TestMarioIsFalling(){

    Rect mar = mario_->GetRectClone();
    Mario::State current_state = mario_->GetState();
    
    mar.move(0,-5);
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
    if (is_floor == nullptr && is_pipe == nullptr && is_pow == nullptr &&
        current_state != Mario::kJump) {
      // 让mario下落
      mario_->SetState(Mario::kFall);
    }
    mar.move(0,5);

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
 private:
  std::vector<Floor*> floor_;
  std::vector<Pow*> pow_;
  std::vector<Pipe*> pipe_;
  std::vector<Turtle*> turtle_;
  Mario* mario_;
  int life_;
  int score_;
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
