/* SPDX-License-Identifier: MIT */
#pragma once

#include <PolkitQt1/Agent/Listener>
#include <PolkitQt1/Agent/Session>

#include <QPointer>
#include <QStringList>

class PolkitAgentModel final : public PolkitQt1::Agent::Listener
{
    Q_OBJECT
    Q_PROPERTY(bool registered READ isRegistered NOTIFY registrationChanged)
    Q_PROPERTY(QString status READ status NOTIFY registrationChanged)
    Q_PROPERTY(bool authenticationActive READ authenticationActive NOTIFY authenticationActiveChanged)
    Q_PROPERTY(QString actionId READ actionId NOTIFY authenticationActiveChanged)
    Q_PROPERTY(QString message READ message NOTIFY authenticationActiveChanged)
    Q_PROPERTY(QStringList identities READ identities NOTIFY authenticationActiveChanged)
    Q_PROPERTY(QString prompt READ prompt NOTIFY promptChanged)
    Q_PROPERTY(bool promptEcho READ promptEcho NOTIFY promptChanged)
    Q_PROPERTY(bool waitingForResponse READ waitingForResponse NOTIFY promptChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)

public:
    explicit PolkitAgentModel(QObject *parent = nullptr);

    bool isRegistered() const;
    QString status() const;
    bool authenticationActive() const;
    QString actionId() const;
    QString message() const;
    QStringList identities() const;
    QString prompt() const;
    bool promptEcho() const;
    bool waitingForResponse() const;
    QString error() const;

    Q_INVOKABLE bool startAuthentication(int identityIndex);
    Q_INVOKABLE void submitResponse(const QString &response);
    Q_INVOKABLE void cancelRequest();

public Q_SLOTS:
    void initiateAuthentication(const QString &actionId,
                                const QString &message,
                                const QString &iconName,
                                const PolkitQt1::Details &details,
                                const QString &cookie,
                                const PolkitQt1::Identity::List &identities,
                                PolkitQt1::Agent::AsyncResult *result) override;
    bool initiateAuthenticationFinish() override;
    void cancelAuthentication() override;

Q_SIGNALS:
    void registrationChanged();
    void authenticationActiveChanged();
    void promptChanged();
    void errorChanged();
    void authenticationCompleted(bool authorized);

private:
    bool m_registered = false;
    QString m_status;
    bool m_authenticationActive = false;
    QString m_actionId;
    QString m_message;
    QStringList m_identityLabels;
    PolkitQt1::Identity::List m_identityValues;
    QString m_cookie;
    PolkitQt1::Agent::AsyncResult *m_result = nullptr;
    QPointer<PolkitQt1::Agent::Session> m_session;
    QString m_prompt;
    bool m_promptEcho = false;
    bool m_waitingForResponse = false;
    QString m_error;

    void finishRequest(bool authorized, const QString &error = {});
    void setError(const QString &error);
};
