/* SPDX-License-Identifier: MIT */
#include "polkitagent.h"

#include <PolkitQt1/Subject>

#include <QCoreApplication>
#include <QDBusConnection>

#include <pwd.h>
#include <unistd.h>

namespace
{
QString identityLabel(const PolkitQt1::Identity &identity)
{
    PolkitQt1::Identity copy = identity;
    const PolkitQt1::UnixUserIdentity user = copy.toUnixUserIdentity();
    if (!user.isValid()) {
        return identity.toString();
    }

    if (const passwd *account = getpwuid(static_cast<uid_t>(user.uid()))) {
        const QString login = QString::fromLocal8Bit(account->pw_name);
        const QString display = QString::fromLocal8Bit(account->pw_gecos).section(QLatin1Char(','), 0, 0).trimmed();
        return display.isEmpty() || display == login ? login : QStringLiteral("%1 (%2)").arg(display, login);
    }
    return QStringLiteral("User %1").arg(user.uid());
}
}

PolkitAgentModel::PolkitAgentModel(QObject *parent)
    : PolkitQt1::Agent::Listener(parent)
{
    if (!QDBusConnection::systemBus().isConnected()) {
        m_status = tr("System D-Bus is unavailable; PolicyKit authentication prompts cannot be registered.");
        return;
    }

    const PolkitQt1::UnixSessionSubject subject(QCoreApplication::applicationPid());
    if (!subject.isValid()) {
        m_status = tr("AeroShell could not identify its login session for authentication.");
        return;
    }

    m_registered = registerListener(subject, QStringLiteral("/org/aeroshell/PolkitAgent"));
    m_status = m_registered ? tr("Authentication prompts are ready for this AeroShell session.")
                            : tr("AeroShell could not register its authentication prompt with PolicyKit.");
}

bool PolkitAgentModel::isRegistered() const
{
    return m_registered;
}

QString PolkitAgentModel::status() const
{
    return m_status;
}

bool PolkitAgentModel::authenticationActive() const
{
    return m_authenticationActive;
}

QString PolkitAgentModel::actionId() const
{
    return m_actionId;
}

QString PolkitAgentModel::message() const
{
    return m_message;
}

QStringList PolkitAgentModel::identities() const
{
    return m_identityLabels;
}

QString PolkitAgentModel::prompt() const
{
    return m_prompt;
}

bool PolkitAgentModel::promptEcho() const
{
    return m_promptEcho;
}

bool PolkitAgentModel::waitingForResponse() const
{
    return m_waitingForResponse;
}

QString PolkitAgentModel::error() const
{
    return m_error;
}

void PolkitAgentModel::initiateAuthentication(const QString &actionId,
                                              const QString &message,
                                              const QString &iconName,
                                              const PolkitQt1::Details &details,
                                              const QString &cookie,
                                              const PolkitQt1::Identity::List &identities,
                                              PolkitQt1::Agent::AsyncResult *result)
{
    Q_UNUSED(iconName)
    Q_UNUSED(details)

    if (!m_registered || m_authenticationActive || !result) {
        if (result) {
            result->setError(tr("AeroShell cannot handle another authentication request right now."));
            result->setCompleted();
        }
        return;
    }
    if (identities.isEmpty()) {
        result->setError(tr("PolicyKit did not provide an identity to authenticate."));
        result->setCompleted();
        return;
    }

    m_result = result;
    m_actionId = actionId;
    m_message = message;
    m_cookie = cookie;
    m_identityValues = identities;
    m_identityLabels.clear();
    for (const PolkitQt1::Identity &identity : identities) {
        m_identityLabels.push_back(identityLabel(identity));
    }
    m_prompt.clear();
    m_error.clear();
    m_promptEcho = false;
    m_waitingForResponse = false;
    m_authenticationActive = true;
    Q_EMIT errorChanged();
    Q_EMIT promptChanged();
    Q_EMIT authenticationActiveChanged();
}

bool PolkitAgentModel::initiateAuthenticationFinish()
{
    return true;
}

void PolkitAgentModel::cancelAuthentication()
{
    if (!m_authenticationActive) {
        return;
    }
    if (m_session) {
        // Session::cancel() emits completed(false); finishRequest() then completes
        // the PolicyKit result exactly once from the normal completion path.
        m_session->cancel();
        return;
    }
    finishRequest(false, tr("Authentication was cancelled."));
}

bool PolkitAgentModel::startAuthentication(int identityIndex)
{
    if (!m_authenticationActive || m_session || !m_result
        || identityIndex < 0 || identityIndex >= m_identityValues.size()) {
        return false;
    }

    auto *session = new PolkitQt1::Agent::Session(m_identityValues.at(identityIndex), m_cookie, m_result, this);
    m_session = session;
    connect(session, &PolkitQt1::Agent::Session::request, this, [this](const QString &request, bool echo) {
        m_prompt = request;
        m_promptEcho = echo;
        m_waitingForResponse = true;
        m_error.clear();
        Q_EMIT errorChanged();
        Q_EMIT promptChanged();
    });
    connect(session, &PolkitQt1::Agent::Session::showError, this, [this](const QString &error) {
        setError(error);
    });
    connect(session, &PolkitQt1::Agent::Session::showInfo, this, [this](const QString &info) {
        m_prompt = info;
        m_promptEcho = true;
        m_waitingForResponse = false;
        Q_EMIT promptChanged();
    });
    connect(session, &PolkitQt1::Agent::Session::completed, this, [this, session](bool authorized) {
        if (m_session != session || !m_authenticationActive) {
            return;
        }
        m_session = nullptr;
        session->deleteLater();
        finishRequest(authorized, authorized ? QString() : tr("Authentication was not accepted."));
    });
    session->initiate();
    return true;
}

void PolkitAgentModel::submitResponse(const QString &response)
{
    if (!m_session || !m_waitingForResponse) {
        return;
    }
    m_waitingForResponse = false;
    m_prompt.clear();
    Q_EMIT promptChanged();
    m_session->setResponse(response);
}

void PolkitAgentModel::cancelRequest()
{
    cancelAuthentication();
}

void PolkitAgentModel::finishRequest(bool authorized, const QString &error)
{
    if (!m_authenticationActive) {
        return;
    }
    if (!error.isEmpty()) {
        setError(error);
    }
    if (m_result) {
        m_result->setCompleted();
        m_result = nullptr;
    }
    m_cookie.clear();
    m_actionId.clear();
    m_message.clear();
    m_identityLabels.clear();
    m_identityValues.clear();
    m_prompt.clear();
    m_promptEcho = false;
    m_waitingForResponse = false;
    m_authenticationActive = false;
    Q_EMIT promptChanged();
    Q_EMIT authenticationActiveChanged();
    Q_EMIT authenticationCompleted(authorized);
}

void PolkitAgentModel::setError(const QString &error)
{
    m_error = error;
    Q_EMIT errorChanged();
}
