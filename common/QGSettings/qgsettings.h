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
     * @brief setEnum
     * @param key
     * @param value
     * 在设置中查找枚举类型的nick以获取值并将其写入键。
     * 给出一个不包含在架构中的键来进行设置或未将其标记为枚举类型，
     * 或者将值设为不是该命名类型的有效值是程序员的错误。
     * 执行写操作后，直接使用g_settings_get_string（）
     * 访问密钥将返回与值关联的'nick'。
     */
    void setEnum(char *key,int value);

    /**
      * 获取存储在key设置中的值，并将其转换为它表示的枚举值。
      * 为了使用此函数，值的类型必须为字符串，并且必须在模式文件中将其标记为枚举类型。
      * 给出不包含在设置模式中或未标记为枚举类型的密钥是程序员的错误。
      * 如果配置数据库中存储的值不是该枚举类型的有效值，则此函数将返回默认值。
      */
    int getEnum(const char *key);

    /**
     * g_settings_get()字符串数组的便捷变体。
     * 给出一个key 未指定为的架构中的字符串类型数组是程序员的错误settings 。
     * return: 一个新分配的，NULL终止的字符串数组，该值存储在key 中settings
     */
    char **getStrv(const char *key);

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
