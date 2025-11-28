#include <QSortFilterProxyModel>
//#include <QMap>
#include <QTime>
#include <QStringList>

class QScheduleSortModel : public QSortFilterProxyModel {
public:
    QScheduleSortModel(QObject *parent = nullptr) : QSortFilterProxyModel(parent) {}

protected:
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override {
        QVariant leftData = sourceModel()->data(left);
        QVariant rightData = sourceModel()->data(right);

        // Список дней недели в нужном порядке
        static const QStringList dayOrder = {
            "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"
        };
        QString header = sourceModel()->headerData(left.column(), Qt::Horizontal).toString();

        if (header == "day") {
            int leftIndex = dayOrder.indexOf(leftData.toString());
            int rightIndex = dayOrder.indexOf(rightData.toString());
            return leftIndex < rightIndex;
        } else if (header == "time") {
            QTime t1 = QTime::fromString(leftData.toString(), "hh:mm");
            QTime t2 = QTime::fromString(rightData.toString(), "hh:mm");
            return t1 < t2;
        }

        // иначе обычное сравнение
        return QSortFilterProxyModel::lessThan(left, right);
    }
public:
    QMap<int, QSet<QString>> allowValues;

    void setAllowedValues(int column, const QSet<QString>& values) {
        if (values.isEmpty()) allowValues.remove(column);
        else allowValues[column] = values;
        invalidateFilter();
    }

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override {
        Q_UNUSED(sourceParent);
        for (auto it = allowValues.constBegin(); it != allowValues.constEnd(); ++it) {
            int col = it.key();
            const QSet<QString>& allowed = it.value();
            if (allowed.isEmpty()) continue;
            QModelIndex idx = sourceModel()->index(sourceRow, col);
            QString cell = sourceModel()->data(idx).toString();
            if (!allowed.contains(cell))
                return false;
        }
        // если все колонки прошли - строка допускается
        return true;
    }
};
