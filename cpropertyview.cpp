/* Copyright (C) 2018 chipmunk-sm <dannico@linuxmail.org> */

#include "cpropertyview.h"

#include <QMessageBox>
#include <QStandardItemModel>
#include <sstream>
#include <iomanip>

#define GEN_LIST_PARAM\
    MACRO_PROP(int8_t)\
    MACRO_PROP(uint8_t)\
    MACRO_PROP(int16_t)\
    MACRO_PROP(uint16_t)\
    MACRO_PROP(int32_t)\
    MACRO_PROP(uint32_t)\
    MACRO_PROP(int64_t)\
    MACRO_PROP(uint64_t)\
    /*MACRO_PROP(__int128_t)*/\
    /*MACRO_PROP(__uint128_t)*/\
    MACRO_PROP(float)\
    MACRO_PROP(double)\
    MACRO_PROP(long double)

CPropertyView::CPropertyView(QTreeView *propertyView)
{
    m_propertyView = propertyView;
    propertyView->setItemDelegate(new CPropertyViewDelegate(propertyView));
    Init();
}

CPropertyView::~CPropertyView()
{
    if( m_file.isOpen() )
        m_file.close();
}

void CPropertyView::Init()
{

#undef  MACRO_PROP
#define MACRO_PROP(XIDNAME) \
    if( m_buffer.size() < sizeof(XIDNAME))\
    m_buffer.resize(sizeof(XIDNAME));

    GEN_LIST_PARAM;

#undef  MACRO_PROP

    QStringList cols;
    cols << QObject::tr("Type Name")
         << QObject::tr("Size")
         << QObject::tr("Value");

    m_value_column = cols.size() - 1;

    auto pModel = new QStandardItemModel(0, cols.size(), m_propertyView);
    pModel->setHorizontalHeaderLabels(cols);
    m_propertyView->setModel(pModel);

    auto rows = QList<QStringList>()

        #undef  MACRO_PROP
        #define MACRO_PROP(XIDNAME) << (QStringList() << #XIDNAME <<  std::to_string(sizeof(XIDNAME)).c_str())
            GEN_LIST_PARAM
            GEN_LIST_PARAM;
#undef  MACRO_PROP

    rows.append(QStringList() << "Latin1" );
    rows.append(QStringList() << "Utf8" );
    rows.append(QStringList() << "Utf16" );
    rows.append(QStringList() << "Ucs4" );

    foreach (const auto &row, rows)
    {
        QList<QStandardItem *> items;
        foreach (QString text, row)
            items.append(new QStandardItem(text));

        pModel->appendRow(items);
    }
}

void CPropertyView::OpenFile(const QString &path)
{
    if( m_file.isOpen() )
        m_file.close();

    m_file.setFileName(path);

    if (!m_file.open(QIODevice::ReadOnly))
    {
        QMessageBox::critical(m_propertyView, QObject::tr("Open"), m_file.errorString(), QMessageBox::Ok);
        return;
    }
}

void CPropertyView::Close()
{
    if( m_file.isOpen() )
        m_file.close();
    DecodeValue(nullptr, 0);
}

QFile *CPropertyView::GetFileHandler()
{
    return &m_file;
}

void CPropertyView::SetDisplayText(bool displayText)
{
    m_displayText = displayText;
//    if(!displayText)
//        DecodeValue(nullptr, 0);
}

void CPropertyView::DecodeValue(int64_t pos)
{
    if(m_file.isOpen() && pos >= 0 && m_file.seek(pos))
    {

        if( m_buffer.size() < m_string_len)
            m_buffer.resize(m_string_len);

        auto len = m_file.read(reinterpret_cast<char*>(m_buffer.data()), static_cast<int32_t>(m_buffer.size()) );
        if(len >= 0)
        {
            auto maxLen = static_cast<int64_t>(m_buffer.size() - sizeof(uint32_t));
            if(len > maxLen)
                len = maxLen;

            memset(m_buffer.data() + len, 0, m_buffer.size() - static_cast<uint32_t>(len));

            DecodeValue(reinterpret_cast<char*>(m_buffer.data()), static_cast<uint32_t>(len));
            return;
        }
    }

    DecodeValue(nullptr, 0);
}

void CPropertyView::DecodeValue(char *pBuffer, unsigned int bufferSize)
{
    auto pModel = m_propertyView->model();
    auto index = 0;

    const auto hexPrefixEnd = 8;

#undef  MACRO_PROP
#define MACRO_PROP(XIDNAME)\
    try {\
    if(bufferSize >= sizeof(XIDNAME)){\
    std::stringstream ss;\
    if(index < 2){\
    ss << "0x" << std::setw(sizeof(XIDNAME) * 2) << std::setfill('0') << std::uppercase << std::hex << static_cast<uint32_t>(*reinterpret_cast<uint8_t*>(pBuffer));\
}else{\
    ss << (index < hexPrefixEnd ? "0x" : "") << std::setw(sizeof(XIDNAME) * (index < hexPrefixEnd ? 2 : 0)) << std::setfill('0') << std::uppercase  << std::hex << *reinterpret_cast<XIDNAME*>(pBuffer);\
}\
    pModel->setData(pModel->index(index, m_value_column), ss.str().c_str());\
}else{\
    pModel->setData(pModel->index(index, m_value_column), "-");\
}\
} catch (...) {\
    pModel->setData(pModel->index(index, m_value_column), "-");\
}\
    index++;

    GEN_LIST_PARAM;
#undef  MACRO_PROP

#define MACRO_PROP(XIDNAME)\
    try {\
    if(bufferSize >= sizeof(XIDNAME)){\
    std::stringstream ss;\
    ss << std::to_string(*reinterpret_cast<XIDNAME*>(pBuffer));\
    pModel->setData(pModel->index(index, m_value_column), ss.str().c_str());\
}else{\
    pModel->setData(pModel->index(index, m_value_column), "-");\
}\
} catch (...) {\
    pModel->setData(pModel->index(index, m_value_column), "-");\
}\
    index++;

    GEN_LIST_PARAM;
#undef  MACRO_PROP

    QString str;
    if(!m_displayText)
    {
        pModel->setData(pModel->index(index++, m_value_column), str);
        pModel->setData(pModel->index(index++, m_value_column), str);
        pModel->setData(pModel->index(index++, m_value_column), str);
        pModel->setData(pModel->index(index++, m_value_column), str);
        return;
    }

    try { str = QString::fromLatin1(pBuffer); } catch (...){ str = "-"; }
    pModel->setData(pModel->index(index++, m_value_column), str);

    try { str = QString::fromUtf8(pBuffer); } catch (...){ str = "-"; }
    pModel->setData(pModel->index(index++, m_value_column), str);

    if(bufferSize >= sizeof(ushort)) {
        try {str = QString::fromUtf16(reinterpret_cast<ushort*>(pBuffer)); } catch (...){ str = "-"; }
    } else {
        str = "-";
    }
    pModel->setData(pModel->index(index++, m_value_column), str);

    if(bufferSize >= sizeof(uint)) {
        try { str = QString::fromUcs4(reinterpret_cast<uint*>(pBuffer)); } catch (...){ str = "-"; }
    } else {
        str = "-";
    }
    pModel->setData(pModel->index(index++, m_value_column), str);

}
