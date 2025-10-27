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

        // Если сортируем колонку "день"
        if (header == "day") { // например, если колонка 2 = "day"
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
};
