// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#include <gtest/gtest.h>

#include "utils/DocumentBundle.hpp"

#include <QtCore/QDir>
#include <QtCore/QJsonObject>
#include <QtCore/QTemporaryDir>

using Utils::DocumentBundle;

TEST(DocumentBundleTests, CreateAndProbe)
{
    QTemporaryDir temp;
    ASSERT_TRUE(temp.isValid());

    const QString bundlePath = QDir(temp.path()).filePath(QStringLiteral("MyDesign.ironsmith"));

    DocumentBundle::BundleInit init;
    init.name = QStringLiteral("MyDesign");
    init.program = QJsonObject{{QStringLiteral("deviceTarget"), QStringLiteral("npu1col1")}};
    init.design = QJsonObject{{QStringLiteral("blocks"), QJsonArray{}},
                              {QStringLiteral("wires"), QJsonArray{}}};

    const Utils::Result created = DocumentBundle::create(bundlePath, init);
    ASSERT_TRUE(created.ok) << created.errors.join("\n").toStdString();

    QString error;
    ASSERT_TRUE(DocumentBundle::isBundle(bundlePath, &error)) << error.toStdString();

    const auto info = DocumentBundle::probe(bundlePath);
    ASSERT_TRUE(info.valid) << info.error.toStdString();
    EXPECT_EQ(info.name, init.name);
    EXPECT_FALSE(info.documentId.isEmpty());
}

TEST(DocumentBundleTests, NormalizeEnsuresExtension)
{
    const QString normalized = DocumentBundle::normalizeBundlePath(QStringLiteral("/tmp/TestDesign"));
    EXPECT_TRUE(normalized.endsWith(QStringLiteral(".ironsmith")));
}
