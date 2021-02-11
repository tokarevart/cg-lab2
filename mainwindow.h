#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QGraphicsScene>
#include <QMainWindow>
#include <QRandomGenerator>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

  public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

  private slots:
    void on_gen_btn_clicked();

    void on_gen_builtin_btn_clicked();

    void on_thous_gen_builtin_btn_clicked();

    void on_thous_gen_btn_clicked();

    void on_gen_gl_btn_clicked();

    void on_thous_gen_gl_btn_clicked();

  private:
    Ui::MainWindow *ui;
    QGraphicsScene *scene;
    QRandomGenerator rng;
    QOpenGLWidget *glwid;

    QPoint random_point(QRect rect);
    QPolygon random_polygon(QRect rect, int n);
};
#endif // MAINWINDOW_H
