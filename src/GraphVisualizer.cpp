#include "GraphVisualizer.h"
#include <QProcess>
#include <QStandardPaths>
#include <QFileInfo>
#include <QRegularExpression>
#include <QDebug>
#include <cmath>

GraphVisualizer::GraphVisualizer(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
    setAntialiasing(true);

    // Look for dot in standard locations
    QString dot = QStandardPaths::findExecutable("dot.exe");
    if (dot.isEmpty()) {
        dot = QStandardPaths::findExecutable("dot");
    }
    if (dot.isEmpty()) {
        // Try common install paths on Windows
        const QStringList candidates = {
            "C:/Program Files/Graphviz/bin/dot.exe",
            "C:/Program Files (x86)/Graphviz/bin/dot.exe",
            "D:/Program Files/Graphviz/bin/dot.exe"
        };
        for (const auto &c : candidates) {
            if (QFileInfo::exists(c)) {
                dot = c;
                break;
            }
        }
    }

    if (!dot.isEmpty()) {
        m_dotExecutable = dot;
        m_hasGraphviz = true;
        m_statusMessage = "Graphviz dot 引擎就绪: " + dot;
    } else {
        m_hasGraphviz = false;
        m_statusMessage = "未检测到外部 dot 命令，启用内置高性能渲染引擎";
    }
}

void GraphVisualizer::setDotSource(const QString &source) {
    if (m_dotSource == source) return;
    m_dotSource = source;
    emit dotSourceChanged();
    renderDot();
}

void GraphVisualizer::setZoomFactor(qreal factor) {
    if (factor < 0.1) factor = 0.1;
    if (factor > 5.0) factor = 5.0;
    if (qFuzzyCompare(m_zoomFactor, factor)) return;
    m_zoomFactor = factor;
    emit zoomFactorChanged();
    update();
}

void GraphVisualizer::setPanX(qreal x) {
    if (qFuzzyCompare(m_panX, x)) return;
    m_panX = x;
    emit panChanged();
    update();
}

void GraphVisualizer::setPanY(qreal y) {
    if (qFuzzyCompare(m_panY, y)) return;
    m_panY = y;
    emit panChanged();
    update();
}

void GraphVisualizer::resetView() {
    m_zoomFactor = 1.0;
    m_panX = 0.0;
    m_panY = 0.0;
    emit zoomFactorChanged();
    emit panChanged();
    update();
}

void GraphVisualizer::zoomIn() {
    setZoomFactor(m_zoomFactor * 1.2);
}

void GraphVisualizer::zoomOut() {
    setZoomFactor(m_zoomFactor / 1.2);
}

void GraphVisualizer::panBy(qreal dx, qreal dy) {
    m_panX += dx;
    m_panY += dy;
    emit panChanged();
    update();
}

void GraphVisualizer::renderDot() {
    m_svgLoaded = false;
    if (m_dotSource.trimmed().isEmpty()) {
        update();
        return;
    }

    if (m_hasGraphviz && !m_dotExecutable.isEmpty()) {
        QProcess proc;
        proc.start(m_dotExecutable, QStringList() << "-Tsvg");
        if (proc.waitForStarted(2000)) {
            proc.write(m_dotSource.toUtf8());
            proc.closeWriteChannel();
            if (proc.waitForFinished(5000)) {
                m_renderedSvg = proc.readAllStandardOutput();
                if (!m_renderedSvg.isEmpty() && m_svgRenderer.load(m_renderedSvg)) {
                    m_svgLoaded = true;
                    m_statusMessage = "Graphviz SVG 渲染成功";
                    emit statusMessageChanged();
                    update();
                    return;
                }
            }
        }
    }

    // Fallback: Use built-in graph drawing
    m_statusMessage = "使用内置拓扑图引擎渲染";
    emit statusMessageChanged();
    update();
}

void GraphVisualizer::paint(QPainter *painter) {
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setRenderHint(QPainter::SmoothPixmapTransform, true);

    // Dark canvas background
    painter->fillRect(boundingRect(), QColor(24, 24, 27));

    // Save state for Pan & Zoom
    painter->save();
    painter->translate(width() / 2.0 + m_panX, height() / 2.0 + m_panY);
    painter->scale(m_zoomFactor, m_zoomFactor);
    painter->translate(-width() / 2.0, -height() / 2.0);

    if (m_svgLoaded) {
        // Draw SVG centered
        QSize defSize = m_svgRenderer.defaultSize();
        if (defSize.isEmpty()) defSize = QSize(width(), height());
        QRectF bounds((width() - defSize.width()) / 2.0, (height() - defSize.height()) / 2.0,
                      defSize.width(), defSize.height());
        m_svgRenderer.render(painter, bounds);
    } else {
        renderFallbackGraph(painter);
    }

    painter->restore();
}

void GraphVisualizer::renderFallbackGraph(QPainter *painter) {
    if (m_dotSource.isEmpty()) {
        painter->setPen(QColor(150, 150, 150));
        painter->drawText(boundingRect(), Qt::AlignCenter, "暂无图谱数据。请选择文件或分析项目。");
        return;
    }

    // Parse nodes and edges from dot string
    struct Node {
        QString id;
        QString label;
        QColor color;
        QPointF pos;
    };
    QMap<QString, Node> nodes;
    QList<QPair<QString, QString>> edges;

    // Regex for nodes: "xxx" [label="yyy", fillcolor="zzz"];
    static const QRegularExpression rxNode(QStringLiteral("\\s*\"([^\"]+)\"\\s*\\[label=\"([^\"]*)\".*?(?:fillcolor=\"([^\"]*)\")?.*\\];"));
    // Regex for edges: "xxx" -> "yyy";
    static const QRegularExpression rxEdge(QStringLiteral("\\s*\"([^\"]+)\"\\s*->\\s*\"([^\"]+)\";"));

    QStringList lines = m_dotSource.split('\n');
    for (const QString &line : lines) {
        auto mN = rxNode.match(line);
        if (mN.hasMatch()) {
            QString id = mN.captured(1);
            QString label = mN.captured(2).replace("\\n", "\n");
            QString colorStr = mN.captured(3);
            QColor col(colorStr.isEmpty() ? "#007acc" : colorStr);
            nodes[id] = {id, label, col, QPointF()};
            continue;
        }
        auto mE = rxEdge.match(line);
        if (mE.hasMatch()) {
            edges.append({mE.captured(1), mE.captured(2)});
            if (!nodes.contains(mE.captured(1))) {
                nodes[mE.captured(1)] = {mE.captured(1), mE.captured(1), QColor("#007acc"), QPointF()};
            }
            if (!nodes.contains(mE.captured(2))) {
                nodes[mE.captured(2)] = {mE.captured(2), mE.captured(2), QColor("#2b2d30"), QPointF()};
            }
        }
    }

    if (nodes.isEmpty()) {
        painter->setPen(QColor(150, 150, 150));
        painter->drawText(boundingRect(), Qt::AlignCenter, "无法解析图谱节点");
        return;
    }

    // Arrange nodes in circle/layers
    int count = nodes.size();
    qreal centerX = width() / 2.0;
    qreal centerY = height() / 2.0;
    qreal radius = qMin(width(), height()) * 0.35;
    if (radius < 120.0) radius = 120.0;

    int idx = 0;
    for (auto it = nodes.begin(); it != nodes.end(); ++it) {
        qreal angle = 2.0 * 3.1415926535 * idx / count;
        it->pos = QPointF(centerX + radius * std::cos(angle), centerY + radius * std::sin(angle));
        idx++;
    }

    // Draw Edges
    painter->setPen(QPen(QColor(90, 130, 180, 200), 2, Qt::SolidLine, Qt::RoundCap));
    for (const auto &e : edges) {
        if (nodes.contains(e.first) && nodes.contains(e.second)) {
            QPointF p1 = nodes[e.first].pos;
            QPointF p2 = nodes[e.second].pos;
            painter->drawLine(p1, p2);

            // Arrow head
            qreal angle = std::atan2(p2.y() - p1.y(), p2.x() - p1.x());
            qreal arrowSize = 10.0;
            QPointF arrowP1 = p2 - QPointF(arrowSize * std::cos(angle - 0.4), arrowSize * std::sin(angle - 0.4));
            QPointF arrowP2 = p2 - QPointF(arrowSize * std::cos(angle + 0.4), arrowSize * std::sin(angle + 0.4));
            QPolygonF arrow;
            arrow << p2 << arrowP1 << arrowP2;
            painter->setBrush(QColor(90, 130, 180, 220));
            painter->drawPolygon(arrow);
        }
    }

    // Draw Nodes
    for (const auto &node : nodes) {
        QRectF rect(node.pos.x() - 65, node.pos.y() - 25, 130, 50);
        painter->setBrush(node.color);
        painter->setPen(QPen(QColor(255, 255, 255, 180), 1.5));
        painter->drawRoundedRect(rect, 8, 8);

        painter->setPen(Qt::white);
        QFont f = painter->font();
        f.setPointSize(9);
        painter->setFont(f);
        painter->drawText(rect, Qt::AlignCenter, node.label);
    }
}
