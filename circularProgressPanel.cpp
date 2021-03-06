/*
 *@author: 缪庆瑞
 *@date:   2020.04.13
 *@brief:  圆形进度面板
 */
#include "circularProgressPanel.h"
#include <qmath.h>
//#include <QtMath>
#include <QDebug>

//外圆画笔宽度
#define OUTER_CIRCLE_PEN_WIDTH 6
//外圆初始点角度和角度增量（用来确定小圆的初始位置和每次的旋转角度）
#define OUTER_CIRCLE_INITIAL_ANGLE 60  //一点钟方向
#define OUTER_CIRCLE_ANGLE_INCREMENT 6

/*
 *@brief:   构造方法
 *@author:  缪庆瑞
 *@date:    2020.04.13
 *@param:   parent:父对象
 *@param:   fixedWidth:固定宽度
 *@param:   fixedHeight:固定高度
 */
CircularProgressPanel::CircularProgressPanel(QWidget *parent, int fixedWidth, int fixedHeight)
    : QWidget(parent),backgroundPixmap(fixedWidth,fixedHeight)
{
    this->setFixedSize(fixedWidth,fixedHeight);
    //设置圆形占比
    this->setCircleRatio(0.8,0.6);
    //初始化刷新小圆位置的定时器
    this->smallCircleTimer = new QTimer(this);
    connect(smallCircleTimer,SIGNAL(timeout()),this,SLOT(smallCircleTimerSlot()));
    //设置文本画笔与字体
    this->setTextProperty();
    this->setText("0.31mg/l");
}
/*
 *@brief:   设置外圆与内圆占比
 *@author:  缪庆瑞
 *@date:    2020.04.13
 *@param:   outerCircleRatio:外圆占比(0.0-1.0)  即外圆半径占部件可绘制最大圆半径的比值
 *@param:   innerCircleRatio:内圆占比(0.0-1.0)  即内圆半径占部件可绘制最大圆半径的比值
 */
void CircularProgressPanel::setCircleRatio(qreal outerCircleRatio, qreal innerCircleRatio)
{
    //圆心
    circleCenter.setX(this->width()*0.5);
    circleCenter.setY(this->height()*0.5);
    //最大半径
    int maxRadius = (this->width() > this->height() )? this->height()*0.5 : this->width()*0.5;
    //设置外圆和内圆半径
    this->outerCircleRadius = maxRadius * outerCircleRatio;
    this->innerCircleRadius = maxRadius * innerCircleRatio;
    //设置外圆和内圆所在矩形
    outerCircleRect = QRectF(circleCenter.x()-outerCircleRadius,circleCenter.y()-outerCircleRadius,
                             outerCircleRadius*2,outerCircleRadius*2);
    innerCircleRect = QRectF(circleCenter.x()-innerCircleRadius,circleCenter.y()-innerCircleRadius,
                             innerCircleRadius*2,innerCircleRadius*2);
    //绘制背景图
    drawBackgroundPixmap();
    //设置小圆圆心初始位置
    setSmallCircleCenter(OUTER_CIRCLE_INITIAL_ANGLE);
}
/*
 *@brief:   设置圆形进度面板中间显示文本的属性参数
 *@author:  缪庆瑞
 *@date:    2020.04.13
 *@param:   textPen:文本画笔
 *@param:   textFont:文本字体
 */
void CircularProgressPanel::setTextProperty(QPen textPen, QFont textFont)
{
    this->textPen = textPen;
    this->textFont = textFont;
}
/*
 *@brief:   设置圆形进度面板中间显示文本内容
 *@author:  缪庆瑞
 *@date:    2019.6.17
 *@param:   text:文本
 */
void CircularProgressPanel::setText(const QString &text)
{
    this->text = text;
    update();
}
/*
 *@brief:   设置小圆圆心(由外圆半径和角度计算而得)
 *@author:  缪庆瑞
 *@date:    2020.04.14
 *@param:   angle:角度  以3点钟方向为0(360)度，逆时针增加
 */
void CircularProgressPanel::setSmallCircleCenter(int angle)
{
    this->smallCircleCurrentAngle = angle;
    smallCircleCenter.setX(circleCenter.x()+outerCircleRadius*qCos(angle/180.0*M_PI));
    smallCircleCenter.setY(circleCenter.y()-outerCircleRadius*qSin(angle/180.0*M_PI));
    update();
}
/*
 *@brief:   小圆开始移动
 *@author:  缪庆瑞
 *@date:    2020.04.14
 *@param:   ms:移动的时间间隔(单位毫秒)
 */
void CircularProgressPanel::smallCircleStartMove(int ms)
{
    smallCircleTimer->start(ms);
}
/*
 *@brief:   小圆停止移动
 *@author:  缪庆瑞
 *@date:    2020.04.14
 *@param:   returnStartPos:小圆是否返回初始位置
 */
void CircularProgressPanel::smallCircleStopMove(bool returnStartPos)
{
    smallCircleTimer->stop();
    if(returnStartPos)
    {
        setSmallCircleCenter(OUTER_CIRCLE_INITIAL_ANGLE);
    }
}
/*
 *@brief:   绘图事件处理函数(当界面不显示时，调用update并不会触发进入该函数)
 *@author:  缪庆瑞
 *@date:    2020.04.13
 *@param:   event: 传递的绘图事件
 */
void CircularProgressPanel::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);
    //定义painter,设置属性
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);//反锯齿

    //绘制背景图
    painter.drawPixmap(this->rect(),backgroundPixmap);
    //绘制旋转的小圆
    drawRotarySmallCircle(painter);
    //绘制文本
    painter.setPen(textPen);
    painter.setFont(textFont);//文本字体的大小无法通过画笔宽度设置，需要设置相应的字体
    painter.drawText(innerCircleRect,Qt::AlignCenter,text);
}
/*
 *@brief:   绘制背景图
 * 将固定不变的图形提前绘制到pixmap中，然后在paintevent()内调用drawPixmap绘制。相较于直接
 * 在widget上绘制，可以在一定程度上降低不断刷新界面而引起的cpu高占用情况
 *@author:  缪庆瑞
 *@date:    2020.04.15
 */
void CircularProgressPanel::drawBackgroundPixmap()
{
    /************设置画笔画刷***************/
    //外圆(环)画笔
    QPen outerCirclePen;
    outerCirclePen.setColor(QColor("#098BDE"));
    outerCirclePen.setWidthF(OUTER_CIRCLE_PEN_WIDTH);
    //内圆画笔
    QPen innerCirclePen(Qt::NoPen);
    //内圆画刷
    QLinearGradient linearGrad;//线性渐变
    linearGrad.setStart(innerCircleRect.topLeft());
    linearGrad.setFinalStop(innerCircleRect.bottomLeft());
    linearGrad.setColorAt(0,QColor("#0CC4FA"));
    linearGrad.setColorAt(1,QColor("#0874D3"));
    QBrush innerCircleBrush(linearGrad);
    /***********在backgroundPixmap上绘图***************/
    backgroundPixmap.fill(Qt::transparent);
    QPainter painter(&backgroundPixmap);
    painter.setRenderHint(QPainter::Antialiasing);//反锯齿
    //绘制外圆(环)
    painter.setPen(outerCirclePen);
    painter.drawEllipse(outerCircleRect);
    //painter.drawArc(outerCircleRect,0,5760);
    //绘制内圆
    painter.setPen(innerCirclePen);
    painter.setBrush(innerCircleBrush);
    painter.drawEllipse(innerCircleRect);
}
/*
 *@brief:   绘制旋转的小圆
 *@author:  缪庆瑞
 *@date:    2020.04.30
 *@param:   painter:绘图工具
 */
void CircularProgressPanel::drawRotarySmallCircle(QPainter &painter)
{
    /************设置画笔画刷***************/
    //小圆弧画笔(以小圆为中心，在周围绘制一定长度的渐变弧)
    QPen smallCircleArcPen;
    smallCircleArcPen.setWidthF(OUTER_CIRCLE_PEN_WIDTH);
    QConicalGradient conicalGrad;//锥形渐变
    conicalGrad.setCenter(circleCenter);
    conicalGrad.setAngle(smallCircleCurrentAngle);
    conicalGrad.setColorAt(1.0, Qt::white);
    conicalGrad.setColorAt(0.9, QColor("#098BDE"));//与外圆环颜色保持一致
    conicalGrad.setColorAt(0.1, QColor("#098BDE"));//在小圆周围显示一段渐变弧
    conicalGrad.setColorAt(0.0, Qt::white);
    smallCircleArcPen.setBrush(QBrush(conicalGrad));
    //小圆画笔与画刷
    QPen smallCirclePen;
    smallCirclePen.setColor(Qt::white);
    smallCirclePen.setWidthF(2);
    QBrush smallCircleBrush = QBrush(QColor("#1CACF7"));
    /************在设备widget上绘图***************/
    //绘制小圆弧
    painter.setPen(smallCircleArcPen);
    painter.drawArc(outerCircleRect,(smallCircleCurrentAngle-36)*16,72*16);
    //绘制小圆
    painter.setPen(smallCirclePen);
    painter.setBrush(smallCircleBrush);
    painter.drawEllipse(smallCircleCenter,OUTER_CIRCLE_PEN_WIDTH*2,OUTER_CIRCLE_PEN_WIDTH*2);
}
/*
 *@brief:   刷新小圆位置的定时器响应槽
 *@author:  缪庆瑞
 *@date:    2020.04.14
 */
void CircularProgressPanel::smallCircleTimerSlot()
{
    //顺时针转动
    smallCircleCurrentAngle -= OUTER_CIRCLE_ANGLE_INCREMENT;
    smallCircleCurrentAngle = smallCircleCurrentAngle < 0?360-OUTER_CIRCLE_ANGLE_INCREMENT:smallCircleCurrentAngle;
    setSmallCircleCenter(smallCircleCurrentAngle);
}
