#pragma once
#include <QQuickPaintedItem>
#include <QPainter>
#include <QPainterPath>
#include <QMap>
#include <QList>
#include <QString>
#include <QPointF>
#include <QRectF>
#include <QColor>
#include <QVariantMap>

struct InteractiveNode {
    QString id;
    QString label;
    QString subLabel;
    QString fullPath;
    QString type;
    int lineCount = 0;
    int incomingEdges = 0;
    int outgoingEdges = 0;
    QColor color;
    QRectF rect;
    QPointF center;
    bool isHovered = false;
    bool isSelected = false;
};

struct InteractiveEdge {
    QString fromId;
    QString toId;
    QString label;
    QColor color;
};

class InteractiveGraphView : public QQuickPaintedItem {
    Q_OBJECT
    Q_PROPERTY(QString dotSource READ dotSource WRITE setDotSource NOTIFY dotSourceChanged)
    Q_PROPERTY(QString selectedNodeId READ selectedNodeId NOTIFY selectedNodeChanged)
    Q_PROPERTY(QString hoveredNodeId READ hoveredNodeId NOTIFY hoveredNodeChanged)
    Q_PROPERTY(qreal zoomFactor READ zoomFactor WRITE setZoomFactor NOTIFY zoomFactorChanged)
    Q_PROPERTY(qreal panX READ panX WRITE setPanX NOTIFY panChanged)
    Q_PROPERTY(qreal panY READ panY WRITE setPanY NOTIFY panChanged)
    Q_PROPERTY(int nodeCount READ nodeCount NOTIFY graphParsed)
    Q_PROPERTY(int edgeCount READ edgeCount NOTIFY graphParsed)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(QString layoutDirection READ layoutDirection WRITE setLayoutDirection NOTIFY layoutDirectionChanged)

public:
    explicit InteractiveGraphView(QQuickItem *parent = nullptr);

    QString dotSource() const { return m_dotSource; }
    void setDotSource(const QString &source);

    QString selectedNodeId() const { return m_selectedNodeId; }
    QString hoveredNodeId() const { return m_hoveredNodeId; }

    qreal zoomFactor() const { return m_zoomFactor; }
    void setZoomFactor(qreal factor);

    qreal panX() const { return m_panX; }
    void setPanX(qreal x);

    qreal panY() const { return m_panY; }
    void setPanY(qreal y);

    int nodeCount() const { return m_nodes.size(); }
    int edgeCount() const { return m_edges.size(); }
    QString statusMessage() const { return m_statusMessage; }

    QString layoutDirection() const { return m_layoutDirection; }
    void setLayoutDirection(const QString &dir);

    void paint(QPainter *painter) override;

    // View control methods
    Q_INVOKABLE void zoomIn();
    Q_INVOKABLE void zoomOut();
    Q_INVOKABLE void resetView();
    Q_INVOKABLE void fitToView();
    Q_INVOKABLE void panBy(qreal dx, qreal dy);

    // Interaction methods
    Q_INVOKABLE void handleMousePress(qreal mouseX, qreal mouseY);
    Q_INVOKABLE void handleMouseMove(qreal mouseX, qreal mouseY);
    Q_INVOKABLE void handleWheel(qreal mouseX, qreal mouseY, qreal deltaY);
    Q_INVOKABLE void selectNode(const QString &nodeId);

signals:
    void dotSourceChanged();
    void selectedNodeChanged(const QString &nodeId, const QString &fullPath);
    void nodeDoubleClicked(const QString &nodeId, const QString &fullPath);
    void hoveredNodeChanged();
    void zoomFactorChanged();
    void panChanged();
    void graphParsed();
    void statusMessageChanged();
    void layoutDirectionChanged();

private:
    void parseAndLayoutGraph();
    void calculateLayeredLayout();
    QPointF screenToWorld(const QPointF &pt) const;
    QPointF worldToScreen(const QPointF &pt) const;
    QString findNodeAt(const QPointF &worldPt) const;

    QString m_dotSource;
    QString m_selectedNodeId;
    QString m_hoveredNodeId;
    qreal m_zoomFactor = 1.0;
    qreal m_panX = 0.0;
    qreal m_panY = 0.0;
    QString m_statusMessage = "拓扑引擎就绪";
    QString m_layoutDirection = "LR"; // "LR" or "TB"

    QMap<QString, InteractiveNode> m_nodes;
    QList<InteractiveEdge> m_edges;
    QRectF m_contentBounds;

    qint64 m_lastClickTime = 0;
    QString m_lastClickedNode;
};
