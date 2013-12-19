#include "xml_writer.h"

#define BOOST_TEST_MODULE	xml_writer
#include <boost/test/unit_test.hpp>

#define HEADER "<?xml version=\"1.0\" encoding=\"utf-8\"?>"

using namespace std;

BOOST_AUTO_TEST_CASE(empty_xml) {
    stringstream ss;

    xml::writer xw(ss);
    BOOST_CHECK_EQUAL(ss.str(), "");
}

BOOST_AUTO_TEST_CASE(simple_xml_element) {
    stringstream ss;

    xml::writer xw(ss);
    { xml::element tag("tag", xw); }
    BOOST_CHECK_EQUAL(ss.str(), HEADER "<tag/>");
}

BOOST_AUTO_TEST_CASE(simple_xml_element_with_text) {
    stringstream ss;

    xml::writer xw(ss);
    {	xml::element tag("tag", xw);
        tag.contents("hello");
    }
    BOOST_CHECK_EQUAL(ss.str(), HEADER "<tag>hello</tag>");
}

BOOST_AUTO_TEST_CASE(nested_xml_elements_with_text) {
    stringstream ss;

    xml::writer xw(ss);
    {	xml::element tag("tag", xw);
        {	xml::element tag("inner", xw);
            tag.contents("hello");
        }
        { xml::element tag("inner", xw); }
    }
    BOOST_CHECK_EQUAL(ss.str(), HEADER "<tag><inner>hello</inner><inner/></tag>");
}

BOOST_AUTO_TEST_CASE(simple_xml_element_with_attr) {
    stringstream ss;

    xml::writer xw(ss);
    {	xml::element tag("tag", xw);
        tag.attr("name", "fred").attr("wife", "wilma");
        tag.contents("hello");
    }
    BOOST_CHECK_EQUAL(ss.str(), HEADER "<tag name=\"fred\" wife=\"wilma\">hello</tag>");
}

BOOST_AUTO_TEST_CASE(xml_element_with_quotes) {
    stringstream ss;

    xml::writer xw(ss);
    {	xml::element tag("tag", xw);
        tag.attr("name", "\"fred\"").attr("wife", "wilma");
        tag.contents("<hello>");
    }
    BOOST_CHECK_EQUAL(ss.str(), HEADER "<tag name=\"&quot;fred&quot;\" wife=\"wilma\">&lt;hello&gt;</tag>");
}

BOOST_AUTO_TEST_CASE(numeric_attributes) {
    stringstream ss;
    xml::writer xw(ss);
    {	xml::element tag("tag", xw);
        tag.attr("age", 20).attr("height", 2.5).attr("single", false);
    }
    BOOST_CHECK_EQUAL(ss.str(), HEADER "<tag age=\"20\" height=\"2.5\" single=\"0\"/>");
}

BOOST_AUTO_TEST_CASE(cdata) {
    stringstream ss;
    xml::writer xw(ss);
    {   xml::element tag("tag", xw);
        tag.cdata("<hello>");
    }
    BOOST_CHECK_EQUAL(ss.str(), HEADER "<tag><![CDATA[<hello>]]></tag>");
}
