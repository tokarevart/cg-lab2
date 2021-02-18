#include "mainwindow.h"

#include <QElapsedTimer>
#include <QGraphicsItem>
#include <QGraphicsProxyWidget>
#include <QPainter>
#include <QVector2D>
#include <algorithm>

#include "ui_mainwindow.h"

QPoint MainWindow::random_point(QRect rect) {
    int x0, y0, x1, y1;
    rect.getCoords(&x0, &y0, &x1, &y1);
    return QPoint(rng.bounded(x0, x1), rng.bounded(y0, y1));
}

QPolygon MainWindow::random_polygon(QRect rect, int n) {
    QPolygon polygon;
    for (int i = 0; i < n; ++i) {
        polygon << random_point(rect);
    }
    return polygon;
}

std::optional<QPointF> segm_horizline_intersection(QPoint p0, QPoint p1, int y) {
    int p0y = p0.y();
    int p1y = p1.y();
    if (p0y == p1y) {
        return std::nullopt;
    }
    int y_p0y = y - p0y;
    int p1y_p0y = p1y - p0y;
    if (y_p0y == 0) {
        return QPointF(p0);
    } else if (y_p0y == p1y_p0y) {
        return QPointF(p1);
    }
    double t = static_cast<double>(y_p0y) / p1y_p0y;
    if (t < 0.0 || t > 1.0) {
        return std::nullopt;
    } else {
        return QPointF(p0) + t * QPointF(p1 - p0);
    }
}

QList<double> polygon_horiz_intersections(const QPolygon& poly, int y) {
    QList<double> res;
    res.reserve(poly.size());
    auto prev = poly.last();
    for (int i = 0; i < poly.size(); ++i) {
        auto cur = poly[i];
        auto ointer = segm_horizline_intersection(prev, cur, y);
        if (ointer.has_value()) {
            if (cur.y() == y) {
                auto next = poly[0];
                if (i < poly.size() - 1) {
                    next = poly[i + 1];
                }
                if ((cur.y() - prev.y()) * (next.y() - cur.y()) < 0) {
                    res.push_back(ointer.value().x());
                }
            } else {
                res.push_back(ointer.value().x());
            }
        }
        prev = cur;
    }
    std::sort(res.begin(), res.end());
    return res;
}

std::array<double, 2> triangle_horiz_intersections(const QPolygon& poly, int y) {
    std::array<double, 2> res;
    std::size_t res_i = 0;
    auto prev = poly.last();
    for (int i = 0; i < 3; ++i) {
        auto cur = poly[i];
        auto ointer = segm_horizline_intersection(prev, cur, y);
        if (ointer.has_value()) {
            if (cur.y() == y) {
                auto next = poly[0];
                if (i < poly.size() - 1) {
                    next = poly[i + 1];
                }
                if ((cur.y() - prev.y()) * (next.y() - cur.y()) < 0) {
                    res[res_i++] = ointer.value().x();
                }
            } else {
                res[res_i++] = ointer.value().x();
            }
        }
        prev = cur;
    }
    if (res[0] > res[1]) {
        std::swap(res[0], res[1]);
    }
    return res;
}

void draw_polygon(QImage& image, const QPolygon& poly,
                  std::function<QRgb()> rgb) {
    QRgb color = rgb();
    QRgb* bits = reinterpret_cast<QRgb*>(image.bits());
    QRect brect = poly.boundingRect();
    int width = image.width();
    for (int y = brect.top(); y < brect.bottom(); ++y) {
        if (poly.size() == 3) {
            auto xinters = triangle_horiz_intersections(poly, y);
            int xbeg = std::max(static_cast<int>(xinters[0]), 0);
            int xend = std::min(static_cast<int>(xinters[1]) + 1, width);
            QRgb* cur_bits = bits + y * width;
            for (int x = xbeg; x < xend; ++x) {
                cur_bits[x] = color;
            }
        } else {
            auto xinters = polygon_horiz_intersections(poly, y);
            for (int i = 0; i < xinters.size(); i += 2) {
                int xbeg = std::max(static_cast<int>(xinters[i]), 0);
                int xend = std::min(static_cast<int>(xinters[i + 1]) + 1, width);
                QRgb* cur_bits = bits + y * width;
                for (int x = xbeg; x < xend; ++x) {
                    cur_bits[x] = color;
                }
            }
        }
    }
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    scene = new QGraphicsScene();

    ui->graphicsView->setScene(scene);
    ui->graphicsView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->graphicsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::on_gen_btn_clicked() {
    QElapsedTimer t;
    t.start();

    QImage image(ui->graphicsView->size(), QImage::Format_RGB32);
    image.fill(Qt::white);

    auto poly = random_polygon(QRect({0, 0}, ui->graphicsView->size()),
                               ui->n_sbox->value());
    draw_polygon(image, poly, [this]() { return rng.generate(); });
    scene->clear();
    scene->setSceneRect(ui->graphicsView->rect());
    scene->addPixmap(QPixmap::fromImage(image))->setPos(0, 0);
    auto label = ui->time_label;
    label->setText(label->text().split(':')[0] + ": " +
                   QString::number(t.elapsed()) + " ms");
}

void MainWindow::on_gen_builtin_btn_clicked() {
    QElapsedTimer t;
    t.start();
    auto poly = random_polygon(QRect({0, 0}, ui->graphicsView->size()),
                               ui->n_sbox->value());
    QImage image(ui->graphicsView->size(), QImage::Format_RGB32);
    image.fill(Qt::white);
    QPainter painter(&image);
    QColor color = QColor::fromRgb(rng.generate());
    QBrush brush(color);
    QPen pen(Qt::black);
    painter.setBrush(brush);
    painter.setPen(pen);
    painter.drawPolygon(poly);
    painter.end();
    scene->clear();
    scene->setSceneRect(ui->graphicsView->rect());
    scene->addPixmap(QPixmap::fromImage(image))->setPos(0, 0);
    auto label = ui->builtin_time_label;
    label->setText(label->text().split(':')[0] + ": " +
                   QString::number(t.elapsed()) + " ms");
}

void MainWindow::on_thous_gen_btn_clicked() {
    QElapsedTimer t;
    t.start();

    QImage image(ui->graphicsView->size(), QImage::Format_RGB32);
    image.fill(Qt::white);

    for (int poly_i = 0; poly_i < 1000; ++poly_i) {
        auto poly = random_polygon(QRect({0, 0}, ui->graphicsView->size()),
                                   ui->n_sbox->value());
        draw_polygon(image, poly, [this]() { return rng.generate(); });
    }

    scene->clear();
    scene->setSceneRect(ui->graphicsView->rect());
    scene->addPixmap(QPixmap::fromImage(image))->setPos(0, 0);
    auto label = ui->time_label;
    label->setText(label->text().split(':')[0] + ": " +
                   QString::number(t.elapsed()) + " ms");
}

void MainWindow::on_thous_gen_builtin_btn_clicked() {
    QElapsedTimer t;
    t.start();
    QImage image(ui->graphicsView->size(), QImage::Format_RGB32);
    image.fill(Qt::white);
    QPainter painter(&image);
    QPen pen(Qt::black);
    painter.setPen(pen);
    scene->setSceneRect(ui->graphicsView->rect());
    scene->clear();
    for (int i = 0; i < 1000; ++i) {
        auto poly = random_polygon(QRect({0, 0}, ui->graphicsView->size()),
                                   ui->n_sbox->value());
        QColor color = QColor::fromRgb(rng.generate());
        QBrush brush(color);
        painter.setBrush(brush);
        painter.drawPolygon(poly);
    }
    painter.end();
    scene->addPixmap(QPixmap::fromImage(image))->setPos(0, 0);
    auto label = ui->builtin_time_label;
    label->setText(label->text().split(':')[0] + ": " +
                   QString::number(t.elapsed()) + " ms");
}

void MainWindow::on_gen_gl_btn_clicked() {
    QElapsedTimer t;
    t.start();
    auto poly = random_polygon(QRect({0, 0}, ui->graphicsView->size()),
                               ui->n_sbox->value());
    QColor color = QColor::fromRgb(rng.generate());
    QBrush brush(color);
    QPen pen(Qt::black);
    QPainter painter(ui->openGLWidget);
    painter.fillRect(ui->openGLWidget->rect(), QBrush(Qt::white));
    painter.setBrush(brush);
    painter.setPen(pen);
    painter.drawPolygon(poly);
    painter.end();
    auto label = ui->gl_time_label;
    label->setText(label->text().split(':')[0] + ": " +
                   QString::number(t.elapsed()) + " ms");
}

void MainWindow::on_thous_gen_gl_btn_clicked() {
    QElapsedTimer t;
    t.start();
    QPainter painter(ui->openGLWidget);
    painter.fillRect(ui->openGLWidget->rect(), QBrush(Qt::white));
    QPen pen(Qt::black);
    for (int i = 0; i < 1000; ++i) {
        auto poly = random_polygon(QRect({0, 0}, ui->graphicsView->size()),
                                   ui->n_sbox->value());
        QColor color = QColor::fromRgb(rng.generate());
        QBrush brush(color);
        painter.setPen(pen);
        painter.setBrush(brush);
        painter.drawPolygon(poly);
    }
    painter.end();
    auto label = ui->gl_time_label;
    label->setText(label->text().split(':')[0] + ": " +
                   QString::number(t.elapsed()) + " ms");
}
