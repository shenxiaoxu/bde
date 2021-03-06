// baljsn_decoderoptions.cpp                                          -*-C++-*-
#include <baljsn_decoderoptions.h>

#include <bsls_ident.h>
BSLS_IDENT_RCSID(baljsn_decoderoptions_cpp,"$Id$ $CSID$")

#include <baljsn_decoderoptions.h>

#include <bdlat_formattingmode.h>
#include <bdlat_valuetypefunctions.h>

#include <bslim_printer.h>
#include <bsls_assert.h>

#include <bsl_iomanip.h>
#include <bsl_limits.h>
#include <bsl_ostream.h>

///Implementation Note
///- - - - - - - - - -
// This file was generated from a script and was subsequently modified to add
// documentation and to make other changes.  The steps to generate and update
// this file can be found in the 'doc/generating_codec_options.txt' file.

namespace BloombergLP {
namespace baljsn {

                            // --------------------
                            // class DecoderOptions
                            // --------------------

// CONSTANTS

const char DecoderOptions::CLASS_NAME[] = "DecoderOptions";

const int DecoderOptions::DEFAULT_INITIALIZER_MAX_DEPTH = 32;

const bool DecoderOptions::DEFAULT_INITIALIZER_SKIP_UNKNOWN_ELEMENTS = true;

const bdlat_AttributeInfo DecoderOptions::ATTRIBUTE_INFO_ARRAY[] = {
    {
        ATTRIBUTE_ID_MAX_DEPTH,
        "MaxDepth",
        sizeof("MaxDepth") - 1,
        "",
        bdlat_FormattingMode::e_DEC
    },
    {
        ATTRIBUTE_ID_SKIP_UNKNOWN_ELEMENTS,
        "SkipUnknownElements",
        sizeof("SkipUnknownElements") - 1,
        "",
        bdlat_FormattingMode::e_TEXT
    }
};

// CLASS METHODS

const bdlat_AttributeInfo *DecoderOptions::lookupAttributeInfo(
        const char *name,
        int         nameLength)
{
    for (int i = 0; i < 2; ++i) {
        const bdlat_AttributeInfo& attributeInfo =
                    DecoderOptions::ATTRIBUTE_INFO_ARRAY[i];

        if (nameLength == attributeInfo.d_nameLength
        &&  0 == bsl::memcmp(attributeInfo.d_name_p, name, nameLength))
        {
            return &attributeInfo;
        }
    }

    return 0;
}

const bdlat_AttributeInfo *DecoderOptions::lookupAttributeInfo(int id)
{
    switch (id) {
      case ATTRIBUTE_ID_MAX_DEPTH:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_MAX_DEPTH];
      case ATTRIBUTE_ID_SKIP_UNKNOWN_ELEMENTS:
        return &ATTRIBUTE_INFO_ARRAY[ATTRIBUTE_INDEX_SKIP_UNKNOWN_ELEMENTS];
      default:
        return 0;
    }
}

// CREATORS

DecoderOptions::DecoderOptions()
: d_maxDepth(DEFAULT_INITIALIZER_MAX_DEPTH)
, d_skipUnknownElements(DEFAULT_INITIALIZER_SKIP_UNKNOWN_ELEMENTS)
{
}

DecoderOptions::DecoderOptions(const DecoderOptions& original)
: d_maxDepth(original.d_maxDepth)
, d_skipUnknownElements(original.d_skipUnknownElements)
{
}

DecoderOptions::~DecoderOptions()
{
}

// MANIPULATORS

DecoderOptions&
DecoderOptions::operator=(const DecoderOptions& rhs)
{
    if (this != &rhs) {
        d_maxDepth = rhs.d_maxDepth;
        d_skipUnknownElements = rhs.d_skipUnknownElements;
    }

    return *this;
}

void DecoderOptions::reset()
{
    d_maxDepth = DEFAULT_INITIALIZER_MAX_DEPTH;
    d_skipUnknownElements = DEFAULT_INITIALIZER_SKIP_UNKNOWN_ELEMENTS;
}

// ACCESSORS

bsl::ostream& DecoderOptions::print(
        bsl::ostream& stream,
        int           level,
        int           spacesPerLevel) const
{
    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("maxDepth", d_maxDepth);
    printer.printAttribute("skipUnknownElements", d_skipUnknownElements);
    printer.end();
    return stream;
}


}  // close package namespace
}  // close enterprise namespace

// GENERATED BY BLP_BAS_CODEGEN_3.8.24 Fri Feb 17 12:35:40 2017
// USING bas_codegen.pl -m msg --package baljsn --noExternalization -E --noAggregateConversion baljsn.xsd
// SERVICE VERSION
// ----------------------------------------------------------------------------
// Copyright 2015 Bloomberg Finance L.P.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// ----------------------------- END-OF-FILE ----------------------------------
