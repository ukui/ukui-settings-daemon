#ifndef QGSETTINGS_H
#define QGSETTINGS_H

#include <QObject>
#include <QStringList>

/**
 * gsettings 的 key 必须是小写字符和下划线组成
 * 此类将所有key转为驼峰方式
 */

class QGSettings : public QObject
{
    Q_OBJECT
public:
    /* 根据 schemaId 和 path 创建QGSettings对象 */
    explicit QGSettings(const QByteArray& schemaId, const QByteArray& path=QByteArray(), QObject *parent = nullptr);
    ~QGSettings();

    /**
     * 根据key获取值, key不存在则报错
     *
     */
    QVariant get (const QString& key) const;

    /**
     * 根据 key 设定值
     */
    void set (const QString& key, const QVariant& value);

    /**
     * 将QGSettings对象更改为'delay-apply'模式。
     * 在此模式下，对设置的更改不会立即传播到后端，
     * 而是在本地保存，直到调用apply()。
     */
    void delay();

    /**
     * 应用对设置所做的任何更改。
     * 这个函数什么都不做，除非设置是在“延迟-应用”模式;
     * 看到delay ()。在正常情况下，总是立即应用设置。
     */
    void apply();

    /**
     *
     */
    bool trySet (const QString& key, const QVariant& value);

    /**
     * 获取 key 的列表
     */
    QStringList keys() const;

    /**
     *
     */
    QVariantList choices (const QString& key) const;

    /**
     * 将 key 设为默认值
     */
    void reset (const QString& key);

    /**
     * 根据给定 id 检查是否存在schema
     */
    static bool isSchemaInstalled (const QByteArray& schemaId);

Q_SIGNALS:
    /**
     * key 的值改变时发射信号
     *
     */
    void changed (const QString& key);

private:
    struct QGSettingsPrivate* mPriv;
    friend struct QGSettingsPrivate;

};

#endif // QGSETTINGS_H
