#include "InteractiveGraphView.h"
#include "Logger.h"
#include <QRegularExpression>
#include <QDateTime>
#include <QFontMetrics>
#include <cmath>
#include <algorithm>

InteractiveGraphView::InteractiveGraphView(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
    setAntialiasing(true);
    setAcceptedMouseButtons(Qt::AllButtons);
}

void InteractiveGraphView::setDotSource(const QString &source) {
    if (m_dotSource == source) return;
    m_dotSource = source;
    emit dotSourceChanged();
    parseAndLayoutGraph();
    update();
}

void InteractiveGraphView::setZoomFactor(qreal factor) {
    factor = std::clamp(factor, 0.1, 4.0);
    if (qFuzzyCompare(m_zoomFactor, factor)) return;
    m_zoomFactor = factor;
    emit zoomFactorChanged();
    update();
}

void InteractiveGraphView::setPanX(qreal x) {
    if (qFuzzyCompare(m_panX, x)) return;
    m_panX = x;
    emit panChanged();
    update();
}

void InteractiveGraphView::setPanY(qreal y) {
    if (qFuzzyCompare(m_panY, y)) return;
    m_panY = y;
    emit panChanged();
    update();
}

void InteractiveGraphView::setLayoutDirection(const QString &dir) {
    if (m_layoutDirection == dir) return;
    m_layoutDirection = dir;
    emit layoutDirectionChanged();
    calculateLayeredLayout();
    update();
}

void InteractiveGraphView::zoomIn() {
    setZoomFactor(m_zoomFactor * 1.25);
}

void InteractiveGraphView::zoomOut() {
    setZoomFactor(m_zoomFactor / 1.25);
}

void InteractiveGraphView::resetView() {
    m_zoomFactor = 1.0;
    m_panX = 0.0;
    m_panY = 0.0;
    emit zoomFactorChanged();
    emit panChanged();
    update();
}

void InteractiveGraphView::panBy(qreal dx, qreal dy) {
    m_panX += dx;
    m_panY += dy;
    emit panChanged();
    update();
}

void InteractiveGraphView::fitToView() {
    if (m_contentBounds.isEmpty() || width() <= 10 || height() <= 10) return;
    qreal scaleX = (width() - 80) / m_contentBounds.width();
    qreal scaleY = (height() - 80) / m_contentBounds.height();
    qreal scale = qMin(scaleX, scaleY);
    if (scale <= 0) scale = 1.0;
    m_zoomFactor = std::clamp(scale, 0.2, 2.0);

    QPointF center = m_contentBounds.center();
    m_panX = (width() / 2.0) - (center.x() * m_zoomFactor);
    m_panY = (height() / 2.0) - (center.y() * m_zoomFactor);

    emit zoomFactorChanged();
    emit panChanged();
    update();
}

QPointF InteractiveGraphView::screenToWorld(const QPointF &pt) const {
    qreal wx = (pt.x() - (width() / 2.0 + m_panX)) / m_zoomFactor + (width() / 2.0);
    qreal wy = (pt.y() - (height() / 2.0 + m_panY)) / m_zoomFactor + (height() / 2.0);
    return QPointF(wx, wy);
}

QPointF InteractiveGraphView::worldToScreen(const QPointF &pt) const {
    qreal sx = (pt.x() - width() / 2.0) * m_zoomFactor + (width() / 2.0 + m_panX);
    qreal sy = (pt.y() - height() / 2.0) * m_zoomFactor + (height() / 2.0 + m_panY);
    return QPointF(sx, sy);
}

QString InteractiveGraphView::findNodeAt(const QPointF &worldPt) const {
    for (auto it = m_nodes.cbegin(); it != m_nodes.cend(); ++it) {
        if (it.value().rect.contains(worldPt)) {
            return it.key();
        }
    }
    return QString();
}

void InteractiveGraphView::handleMousePress(qreal mouseX, qreal mouseY) {
    QPointF worldPt = screenToWorld(QPointF(mouseX, mouseY));
    QString clicked = findNodeAt(worldPt);

    qint64 now = QDateTime::currentMSecsSinceEpoch();
    bool isDbl = (clicked == m_lastClickedNode && (now - m_lastClickTime) < 350 && !clicked.isEmpty());
    m_lastClickTime = now;
    m_lastClickedNode = clicked;

    if (!clicked.isEmpty()) {
        selectNode(clicked);
        if (isDbl) {
            emit nodeDoubleClicked(clicked, m_nodes[clicked].fullPath);
            Logger::instance()->logInfo("图元交互", QString("双击定位节点: %1").arg(m_nodes[clicked].label));
        }
    }
}

void InteractiveGraphView::handleMouseMove(qreal mouseX, qreal mouseY) {
    QPointF worldPt = screenToWorld(QPointF(mouseX, mouseY));
    QString hovered = findNodeAt(worldPt);

    if (m_hoveredNodeId != hovered) {
        if (!m_hoveredNodeId.isEmpty() && m_nodes.contains(m_hoveredNodeId)) {
            m_nodes[m_hoveredNodeId].isHovered = false;
        }
        m_hoveredNodeId = hovered;
        if (!m_hoveredNodeId.isEmpty() && m_nodes.contains(m_hoveredNodeId)) {
            m_nodes[m_hoveredNodeId].isHovered = true;
        }
        emit hoveredNodeChanged();
        update();
    }
}

void InteractiveGraphView::handleWheel(qreal mouseX, qreal mouseY, qreal deltaY) {
    QPointF mouseBefore = screenToWorld(QPointF(mouseX, mouseY));
    if (deltaY > 0) {
        zoomIn();
    } else {
        zoomOut();
    }
    QPointF mouseAfter = screenToWorld(QPointF(mouseX, mouseY));
    // Zoom toward cursor
    m_panX += (mouseAfter.x() - mouseBefore.x()) * m_zoomFactor;
    m_panY += (mouseAfter.y() - mouseBefore.y()) * m_zoomFactor;
    emit panChanged();
    update();
}

void InteractiveGraphView::selectNode(const QString &nodeId) {
    if (m_selectedNodeId == nodeId) return;

    if (!m_selectedNodeId.isEmpty() && m_nodes.contains(m_selectedNodeId)) {
        m_nodes[m_selectedNodeId].isSelected = false;
    }

    m_selectedNodeId = nodeId;
    if (!m_selectedNodeId.isEmpty() && m_nodes.contains(m_selectedNodeId)) {
        m_nodes[m_selectedNodeId].isSelected = true;
        emit selectedNodeChanged(nodeId, m_nodes[nodeId].fullPath);
        Logger::instance()->logInfo("拓扑交互", QString("选中图元: %1 (入度:%2, 出度:%3)")
                                                  .arg(m_nodes[nodeId].label)
                                                  .arg(m_nodes[nodeId].incomingEdges)
                                                  .arg(m_nodes[nodeId].outgoingEdges));
    } else {
        emit selectedNodeChanged(QString(), QString());
    }
    update();
}

void InteractiveGraphView::parseAndLayoutGraph() {
    m_nodes.clear();
    m_edges.clear();
    m_selectedNodeId.clear();
    m_hoveredNodeId.clear();

    if (m_dotSource.trimmed().isEmpty()) {
        m_statusMessage = "暂无图谱数据";
        emit statusMessageChanged();
        emit graphParsed();
        return;
    }

    // Parse rankdir
    static const QRegularExpression rxRankdir(QStringLiteral("rankdir\\s*=\\s*([A-Za-z]+)"));
    auto mRank = rxRankdir.match(m_dotSource);
    if (mRank.hasMatch()) {
        m_layoutDirection = mRank.captured(1).toUpper();
    }

    // Parse Nodes: "id" [label="...", fillcolor="..."];
    static const QRegularExpression rxNode(QStringLiteral("\\s*\"([^\"]+)\"\\s*\\[label=\"([^\"]*)\".*?(?:fillcolor=\"([^\"]*)\")?.*?\\];"));
    // Parse Edges: "id1" -> "id2";
    static const QRegularExpression rxEdge(QStringLiteral("\\s*\"([^\"]+)\"\\s*->\\s*\"([^\"]+)\";"));

    QStringList lines = m_dotSource.split('\n');
    for (const QString &line : lines) {
        auto mN = rxNode.match(line);
        if (mN.hasMatch()) {
            QString id = mN.captured(1);
            QString rawLabel = mN.captured(2);
            QString colorStr = mN.captured(3);

            InteractiveNode node;
            node.id = id;
            node.fullPath = id;

            QStringList parts = rawLabel.split(QStringLiteral("\\n"));
            node.label = parts.value(0, id);
            node.subLabel = (parts.size() > 1) ? parts.value(1) : "";

            if (colorStr.isEmpty()) {
                node.color = QColor("#3b82f6");
            } else {
                node.color = QColor(colorStr);
            }
            m_nodes[id] = node;
            continue;
        }

        auto mE = rxEdge.match(line);
        if (mE.hasMatch()) {
            QString fromId = mE.captured(1);
            QString toId = mE.captured(2);

            InteractiveEdge edge;
            edge.fromId = fromId;
            edge.toId = toId;
            edge.color = QColor("#64748b");
            m_edges.append(edge);

            // Ensure nodes exist
            if (!m_nodes.contains(fromId)) {
                InteractiveNode n;
                n.id = fromId;
                n.label = fromId;
                n.color = QColor("#475569");
                m_nodes[fromId] = n;
            }
            if (!m_nodes.contains(toId)) {
                InteractiveNode n;
                n.id = toId;
                n.label = toId;
                n.color = QColor("#475569");
                m_nodes[toId] = n;
            }
            m_nodes[fromId].outgoingEdges++;
            m_nodes[toId].incomingEdges++;
        }
    }

    calculateLayeredLayout();

    m_statusMessage = QString("成功解析 %1 个节点，%2 条拓扑关系").arg(m_nodes.size()).arg(m_edges.size());
    emit statusMessageChanged();
    emit graphParsed();
}

void InteractiveGraphView::calculateLayeredLayout() {
    if (m_nodes.isEmpty()) {
        m_contentBounds = QRectF();
        return;
    }

    // Determine layers using topological/longest-path heuristic (Sugiyama approach)
    QMap<QString, int> nodeLayer;
    QMap<QString, QList<QString>> outgoingMap;
    for (const auto &e : m_edges) {
        outgoingMap[e.fromId].append(e.toId);
    }

    // Initialize root nodes (incoming = 0) to layer 0
    QList<QString> queue;
    for (auto it = m_nodes.begin(); it != m_nodes.end(); ++it) {
        if (it.value().incomingEdges == 0) {
            nodeLayer[it.key()] = 0;
            queue.append(it.key());
        }
    }

    // If cycle or no roots, start with first node
    if (queue.isEmpty() && !m_nodes.isEmpty()) {
        QString first = m_nodes.begin().key();
        nodeLayer[first] = 0;
        queue.append(first);
    }

    // BFS layer assignment
    while (!queue.isEmpty()) {
        QString curr = queue.takeFirst();
        int currL = nodeLayer.value(curr, 0);

        for (const QString &next : outgoingMap[curr]) {
            int nextL = nodeLayer.value(next, -1);
            if (currL + 1 > nextL) {
                nodeLayer[next] = currL + 1;
                queue.append(next);
            }
        }
    }

    // Fallback unassigned nodes
    for (auto it = m_nodes.begin(); it != m_nodes.end(); ++it) {
        if (!nodeLayer.contains(it.key())) {
            nodeLayer[it.key()] = 0;
        }
    }

    // Group nodes by layer
    QMap<int, QList<QString>> layers;
    int maxLayer = 0;
    for (auto it = nodeLayer.begin(); it != nodeLayer.end(); ++it) {
        layers[it.value()].append(it.key());
        if (it.value() > maxLayer) maxLayer = it.value();
    }

    // Compute dimensions and positions
    const qreal nodeWidth = 160.0;
    const qreal nodeHeight = 56.0;
    const qreal layerSpacingX = 240.0;
    const qreal nodeSpacingY = 85.0;

    qreal minX = 1e9, minY = 1e9, maxX = -1e9, maxY = -1e9;

    bool isLR = (m_layoutDirection == "LR");

    for (int l = 0; l <= maxLayer; ++l) {
        const QList<QString> &nodesInLayer = layers[l];
        int countInLayer = nodesInLayer.size();
        qreal totalLength = countInLayer * nodeSpacingY;

        for (int i = 0; i < countInLayer; ++i) {
            QString nId = nodesInLayer[i];
            InteractiveNode &node = m_nodes[nId];

            qreal cx = 0.0, cy = 0.0;
            if (isLR) {
                cx = 100.0 + l * layerSpacingX;
                cy = 100.0 + (i * nodeSpacingY) - (totalLength / 2.0) + 200.0;
            } else {
                cx = 100.0 + (i * layerSpacingX * 0.8) - ((countInLayer * layerSpacingX * 0.8) / 2.0) + 300.0;
                cy = 100.0 + l * nodeSpacingY * 1.5;
            }

            node.center = QPointF(cx, cy);
            node.rect = QRectF(cx - nodeWidth / 2.0, cy - nodeHeight / 2.0, nodeWidth, nodeHeight);

            minX = qMin(minX, node.rect.left());
            minY = qMin(minY, node.rect.top());
            maxX = qMax(maxX, node.rect.right());
            maxY = qMax(maxY, node.rect.bottom());
        }
    }

    m_contentBounds = QRectF(minX, minY, maxX - minX, maxY - minY);
    fitToView();
}

void InteractiveGraphView::paint(QPainter *painter) {
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter->setRenderHint(QPainter::TextAntialiasing, true);

    // Deep modern dark background
    painter->fillRect(boundingRect(), QColor(18, 18, 22));

    // Draw Subtle Grid
    painter->save();
    painter->translate(width() / 2.0 + m_panX, height() / 2.0 + m_panY);
    painter->scale(m_zoomFactor, m_zoomFactor);
    painter->translate(-width() / 2.0, -height() / 2.0);

    QPen gridPen(QColor(36, 36, 44, 180), 1, Qt::DotLine);
    painter->setPen(gridPen);
    const int gridSize = 60;
    qreal startX = -1000;
    qreal endX = width() + 2000;
    qreal startY = -1000;
    qreal endY = height() + 2000;
    for (qreal x = startX; x <= endX; x += gridSize) {
        painter->drawLine(QPointF(x, startY), QPointF(x, endY));
    }
    for (qreal y = startY; y <= endY; y += gridSize) {
        painter->drawLine(QPointF(startX, y), QPointF(endX, y));
    }

    if (m_nodes.isEmpty()) {
        painter->restore();
        painter->setPen(QColor(115, 115, 128));
        QFont f = painter->font();
        f.setPointSize(11);
        painter->setFont(f);
        painter->drawText(boundingRect(), Qt::AlignCenter, "暂无拓扑关系。请在左侧选择文件或打开项目进行分析。");
        return;
    }

    // 1. Draw Edges with smooth Bézier curves
    for (const auto &e : m_edges) {
        if (!m_nodes.contains(e.fromId) || !m_nodes.contains(e.toId)) continue;
        const auto &n1 = m_nodes[e.fromId];
        const auto &n2 = m_nodes[e.toId];

        bool isRelatedToSelected = (!m_selectedNodeId.isEmpty() && (e.fromId == m_selectedNodeId || e.toId == m_selectedNodeId));

        QColor edgeColor = isRelatedToSelected ? QColor("#38bdf8") : QColor(100, 116, 139, 160);
        qreal lineWidth = isRelatedToSelected ? 2.5 : 1.5;

        painter->setPen(QPen(edgeColor, lineWidth, Qt::SolidLine, Qt::RoundCap));

        QPointF p1, p2;
        if (m_layoutDirection == "LR") {
            p1 = QPointF(n1.rect.right(), n1.center.y());
            p2 = QPointF(n2.rect.left(), n2.center.y());
            qreal dx = (p2.x() - p1.x()) * 0.5;
            QPainterPath path(p1);
            path.cubicTo(QPointF(p1.x() + dx, p1.y()), QPointF(p2.x() - dx, p2.y()), p2);
            painter->strokePath(path, painter->pen());
        } else {
            p1 = QPointF(n1.center.x(), n1.rect.bottom());
            p2 = QPointF(n2.center.x(), n2.rect.top());
            qreal dy = (p2.y() - p1.y()) * 0.5;
            QPainterPath path(p1);
            path.cubicTo(QPointF(p1.x(), p1.y() + dy), QPointF(p2.x(), p2.y() - dy), p2);
            painter->strokePath(path, painter->pen());
        }

        // Draw Arrowhead
        qreal angle = (m_layoutDirection == "LR") ? 0.0 : (3.14159 / 2.0);
        qreal arrowSize = isRelatedToSelected ? 9.0 : 7.0;
        QPointF a1 = p2 - QPointF(arrowSize * std::cos(angle - 0.5), arrowSize * std::sin(angle - 0.5));
        QPointF a2 = p2 - QPointF(arrowSize * std::cos(angle + 0.5), arrowSize * std::sin(angle + 0.5));
        QPolygonF arrow;
        arrow << p2 << a1 << a2;
        painter->setBrush(edgeColor);
        painter->drawPolygon(arrow);
    }

    // 2. Draw Nodes
    for (const auto &node : m_nodes) {
        QRectF r = node.rect;

        // Shadow / Glow effect
        if (node.isSelected) {
            painter->setPen(Qt::NoPen);
            painter->setBrush(QColor(56, 189, 248, 60));
            painter->drawRoundedRect(r.adjusted(-6, -6, 6, 6), 12, 12);
        } else if (node.isHovered) {
            painter->setPen(Qt::NoPen);
            painter->setBrush(QColor(255, 255, 255, 25));
            painter->drawRoundedRect(r.adjusted(-4, -4, 4, 4), 10, 10);
        }

        // Node card background
        QColor fill = node.isSelected ? QColor(30, 41, 59) : (node.isHovered ? QColor(36, 40, 50) : QColor(26, 29, 36));
        painter->setBrush(fill);

        QPen borderPen;
        if (node.isSelected) {
            borderPen = QPen(QColor("#38bdf8"), 2.0);
        } else if (node.isHovered) {
            borderPen = QPen(QColor("#94a3b8"), 1.5);
        } else {
            borderPen = QPen(QColor(60, 66, 80), 1.0);
        }
        painter->setPen(borderPen);
        painter->drawRoundedRect(r, 8, 8);

        // Left accent badge
        QRectF badge(r.left(), r.top(), 5, r.height());
        painter->setPen(Qt::NoPen);
        painter->setBrush(node.color);
        painter->drawRoundedRect(badge, 4, 4);

        // Labels
        painter->setPen(node.isSelected ? QColor("#ffffff") : QColor("#e2e8f0"));
        QFont titleFont = painter->font();
        titleFont.setPointSize(10);
        titleFont.setBold(true);
        painter->setFont(titleFont);

        QRectF textRect = r.adjusted(12, 6, -8, -(node.subLabel.isEmpty() ? 0 : 20));
        painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter,
                          painter->fontMetrics().elidedText(node.label, Qt::ElideMiddle, textRect.width()));

        if (!node.subLabel.isEmpty()) {
            painter->setPen(QColor("#94a3b8"));
            QFont subFont = painter->font();
            subFont.setPointSize(8);
            subFont.setBold(false);
            painter->setFont(subFont);

            QRectF subRect(r.left() + 12, r.top() + r.height() / 2.0, r.width() - 16, r.height() / 2.0 - 4);
            painter->drawText(subRect, Qt::AlignLeft | Qt::AlignVCenter, node.subLabel);
        }
    }

    painter->restore();
}
