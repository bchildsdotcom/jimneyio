#include "inclinometer.hpp"
#include "jimney.hpp"

int testPitch = 0;
int testRoll = 0;
const int MAX_TEST_PITCH = 16;
const int MAX_TEST_ROLL = 5;
bool isPitchReversing = false;
bool isRollReversing = false;

// allows for exagerating changes in pitch and roll for ease of reading
const int ROLL_SCALING = 2;
const int PITCH_SCALING = 2;

Orientation generateTestOrientationData() {
  if(testPitch < MAX_TEST_PITCH && !isPitchReversing) {
    testPitch += 2;
    if(testPitch == MAX_TEST_PITCH) isPitchReversing = true;
  }
  else if(testPitch > -1*MAX_TEST_PITCH && isPitchReversing) 
  {
    testPitch -= 2;
    if(testPitch == -1*MAX_TEST_PITCH) isPitchReversing = false;
  }

  if(testRoll < MAX_TEST_ROLL && !isRollReversing) {
    testRoll += 1;
    if(testRoll == MAX_TEST_ROLL) isRollReversing = true;
  }
  else if(testRoll > -1*MAX_TEST_ROLL && isRollReversing) 
  {
    testRoll -= 1;
    if(testRoll == -1*MAX_TEST_ROLL) isRollReversing = false;
  }

  return Orientation(testPitch, testRoll);  
}


int orientationSamplesCount = 0;
float averageRoll = 0;
float averagePitch = 0; 
Orientation calculateOrientation() {
  auto current = generateTestOrientationData();

  if(orientationSamplesCount < 5) {
    orientationSamplesCount++;
    averageRoll += current.roll;
    averageRoll /= orientationSamplesCount;

    averagePitch += current.pitch;
    averagePitch /= orientationSamplesCount;

    return Orientation(0,0);
  } 

  averageRoll = averageRoll * 0.8 + current.roll * 0.2;
  averagePitch = averagePitch * 0.8 + current.pitch * 0.2;
  return Orientation(
    averagePitch * PITCH_SCALING,
    averageRoll * ROLL_SCALING);
}

Line rotateLine(Line line, float deg) {
   float theta = deg * M_PI / 180;
   float cosang = cos(theta);
   float sinang = sin(theta);
   
   auto cx = (line.p1.x + line.p2.x) / 2;
   auto cy = (line.p1.y + line.p2.y);
  
   auto tx1 = line.p1.x - cx;
   auto ty1 = line.p1.y - cy;

   auto p1 = Point(tx1*cosang + ty1*sinang + cx, -tx1*sinang+ty1*cosang + cy);

   auto tx2 = line.p2.x - cx;
   auto ty2 = line.p2.y - cy;

   auto p2 = Point(tx2*cosang + ty2*sinang + cx, -tx2*sinang+ty2*cosang + cy);

   return Line(p1, p2);
}

int det(Point a, Point b)
{
  return a.x * b.y - a.y*b.x;
}

Point lineIntersection(Line l1, Line l2) 
{
  auto xDiff = Point(l1.p1.x - l1.p2.x,l2.p1.x - l2.p2.x);
  auto yDiff = Point(l1.p1.y - l1.p2.y,l2.p1.y - l2.p2.y);
  
  auto div = det(xDiff, yDiff);
  if (div == 0)
    return Point(INT_MAX, INT_MAX);

  auto d = Point(det(l1.p1, l1.p2), det(l2.p1, l2.p2));
  return Point(det(d, xDiff) / div, det(d, yDiff) / div);
}

void renderInclinometerFrame(PicoGraphics& graphics, Pens& pens) {

  auto orientation = calculateOrientation();

  uint8_t yOffset = 120+orientation.pitch;

  Line line = rotateLine(Line(Point(0, yOffset),Point(240, yOffset)), orientation.roll);
  Point leftEdgePoint = line.p1;
  Point rightEdgePoint = line.p2;
  leftEdgePoint = lineIntersection(line, Line(Point(0,0),Point(0,240)));
  rightEdgePoint = lineIntersection(line, Line(Point(240, 0), Point(240, 240)));

  graphics.set_pen(pens.SKY_BLUE_DAY);
  graphics.clear();

  graphics.set_pen(pens.GRASS_GREEN_DAY);

  std::vector<Point> poly;
  poly.push_back(leftEdgePoint);
  poly.push_back(rightEdgePoint);
  poly.push_back(Point(240,240));
  poly.push_back(Point(0,240));

  graphics.polygon(poly);

  Pen cartesianLinesPen = pens.BLACK;
  Pen spriteTransparencyPen = pens.SPRITE_TRANSPARENCY_DARK;

  graphics.set_pen(cartesianLinesPen);
  graphics.line(Point(120, 0), Point(120, 70));
  graphics.line(Point(120, 170), Point(120, 240));
  graphics.line(Point(0, 120), Point(70, 120));
  graphics.line(Point(170, 120), Point(240, 120));

  drawJimny(graphics, 56, 56, jimny_icon_dark_v9, spriteTransparencyPen);
}