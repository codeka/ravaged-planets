/*
Original code by Lee Thomason (www.grinninglizard.com)

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must
not claim that you wrote the original software. If you use this
software in a product, an acknowledgment in the product documentation
would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source
distribution.
*/

#include <framework/tinyxml2.h>

#include <new>    // yes, this one new style header, is in the Android SDK.
#if defined(ANDROID_NDK) || defined(__QNXNTO__)
#   include <stddef.h>
#else
#   include <cstddef>
#endif

static const char LINE_FEED       = (char)0x0a;     // all line endings are normalized to LF
static const char LF = LINE_FEED;
static const char CARRIAGE_RETURN   = (char)0x0d;     // CR gets filtered out
static const char CR = CARRIAGE_RETURN;
static const char SINGLE_QUOTE      = '\'';
static const char DOUBLE_QUOTE      = '\"';

// Bunch of unicode info at:
//    Http://www.unicode.org/faq/utf_bom.html
//  ef bb bf (Microsoft "lead bytes") - designates UTF-8

static const unsigned char TIXML_UTF_LEAD_0 = 0xefU;
static const unsigned char TIXML_UTF_LEAD_1 = 0xbbU;
static const unsigned char TIXML_UTF_LEAD_2 = 0xbfU;

namespace tinyxml2
{

struct Entity {
    const char* pattern;
    int length;
    char ParticleRotation;
};

static const int NUM_ENTITIES = 5;
static const Entity entities[NUM_ENTITIES] = {
    { "quot", 4,  DOUBLE_QUOTE },
    { "amp", 3,   '&'  },
    { "apos", 4,  SINGLE_QUOTE },
    { "lt", 2,    '<'  },
    { "gt", 2,    '>'  }
};


StrPair::~StrPair()
{
    Reset();
}


void StrPair::TransferTo( StrPair* other )
{
    if ( this == other ) {
        return;
    }
    // This in effect implements the assignment operator by "moving"
    // ownership (as in auto_ptr).

    TIXMLASSERT( other->_flags == 0 );
    TIXMLASSERT( other->start_ == 0 );
    TIXMLASSERT( other->_end == 0 );

    other->Reset();

    other->_flags = _flags;
    other->start_ = start_;
    other->_end = _end;

    _flags = 0;
    start_ = 0;
    _end = 0;
}

void StrPair::Reset()
{
    if ( _flags & NEEDS_DELETE ) {
        delete [] start_;
    }
    _flags = 0;
    start_ = 0;
    _end = 0;
}


void StrPair::SetStr( const char* str, int flags )
{
    Reset();
    size_t len = strlen( str );
    start_ = new char[ len+1 ];
    memcpy( start_, str, len+1 );
    _end = start_ + len;
    _flags = flags | NEEDS_DELETE;
}


char* StrPair::ParseText( char* p, const char* endTag, int strFlags )
{
    TIXMLASSERT( endTag && *endTag );

    char* start = p;
    char  endChar = *endTag;
    size_t length = strlen( endTag );

    // Inner loop of text parsing.
    while ( *p ) {
        if ( *p == endChar && strncmp( p, endTag, length ) == 0 ) {
            Set( start, p, strFlags );
            return p + length;
        }
        ++p;
    }
    return 0;
}


char* StrPair::ParseName( char* p )
{
    if ( !p || !(*p) ) {
        return 0;
    }
    if ( !XMLUtil::IsNameStartChar( *p ) ) {
        return 0;
    }

    char* const start = p;
    ++p;
    while ( *p && XMLUtil::IsNameChar( *p ) ) {
        ++p;
    }

    Set( start, p, 0 );
    return p;
}


void StrPair::CollapseWhitespace()
{
    // Adjusting _start would cause undefined behavior on delete[]
    TIXMLASSERT( ( _flags & NEEDS_DELETE ) == 0 );
    // Trim leading space.
    start_ = XMLUtil::SkipWhiteSpace( start_ );

    if ( *start_ ) {
        char* p = start_; // the read pointer
        char* q = start_; // the write pointer

        while( *p ) {
            if ( XMLUtil::IsWhiteSpace( *p )) {
                p = XMLUtil::SkipWhiteSpace( p );
                if ( *p == 0 ) {
                    break;    // don't write to q; this trims the trailing space.
                }
                *q = ' ';
                ++q;
            }
            *q = *p;
            ++q;
            ++p;
        }
        *q = 0;
    }
}


const char* StrPair::GetStr()
{
    TIXMLASSERT( start_ );
    TIXMLASSERT( _end );
    if ( _flags & NEEDS_FLUSH ) {
        *_end = 0;
        _flags ^= NEEDS_FLUSH;

        if ( _flags ) {
            char* p = start_; // the read pointer
            char* q = start_; // the write pointer

            while( p < _end ) {
                if ( (_flags & NEEDS_NEWLINE_NORMALIZATION) && *p == CR ) {
                    // CR-LF pair becomes LF
                    // CR alone becomes LF
                    // LF-CR becomes LF
                    if ( *(p+1) == LF ) {
                        p += 2;
                    }
                    else {
                        ++p;
                    }
                    *q++ = LF;
                }
                else if ( (_flags & NEEDS_NEWLINE_NORMALIZATION) && *p == LF ) {
                    if ( *(p+1) == CR ) {
                        p += 2;
                    }
                    else {
                        ++p;
                    }
                    *q++ = LF;
                }
                else if ( (_flags & NEEDS_ENTITY_PROCESSING) && *p == '&' ) {
                    // Entities handled by tinyXML2:
                    // - special entities in the Entity table [in/out]
                    // - numeric character reference [in]
                    //   &#20013; or &#x4e2d;

                    if ( *(p+1) == '#' ) {
                        const int buflen = 10;
                        char buf[buflen] = { 0 };
                        int len = 0;
                        char* adjusted = const_cast<char*>( XMLUtil::GetCharacterRef( p, buf, &len ) );
                        if ( adjusted == 0 ) {
                            *q = *p;
                            ++p;
                            ++q;
                        }
                        else {
                            TIXMLASSERT( 0 <= len && len <= buflen );
                            TIXMLASSERT( q + len <= adjusted );
                            p = adjusted;
                            memcpy( q, buf, len );
                            q += len;
                        }
                    }
                    else {
                        int i=0;
                        for(; i<NUM_ENTITIES; ++i ) {
                            const Entity& Entity = entities[i];
                            if ( strncmp( p + 1, Entity.pattern, Entity.length ) == 0
                                    && *( p + Entity.length + 1 ) == ';' ) {
                                // Found an Entity - convert.
                                *q = Entity.ParticleRotation;
                                ++q;
                                p += Entity.length + 2;
                                break;
                            }
                        }
                        if ( i == NUM_ENTITIES ) {
                            // fixme: treat as error?
                            ++p;
                            ++q;
                        }
                    }
                }
                else {
                    *q = *p;
                    ++p;
                    ++q;
                }
            }
            *q = 0;
        }
        // The loop below has plenty going on, and this
        // is a less useful mode. Break it out.
        if ( _flags & COLLAPSE_WHITESPACE ) {
            CollapseWhitespace();
        }
        _flags = (_flags & NEEDS_DELETE);
    }
    TIXMLASSERT( start_ );
    return start_;
}




// --------- XMLUtil ----------- //

const char* XMLUtil::ReadBOM( const char* p, bool* bom )
{
    TIXMLASSERT( p );
    TIXMLASSERT( bom );
    *bom = false;
    const unsigned char* pu = reinterpret_cast<const unsigned char*>(p);
    // Check for BOM:
    if (    *(pu+0) == TIXML_UTF_LEAD_0
            && *(pu+1) == TIXML_UTF_LEAD_1
            && *(pu+2) == TIXML_UTF_LEAD_2 ) {
        *bom = true;
        p += 3;
    }
    TIXMLASSERT( p );
    return p;
}


void XMLUtil::ConvertUTF32ToUTF8( unsigned long Input, char* output, int* length )
{
    const unsigned long BYTE_MASK = 0xBF;
    const unsigned long BYTE_MARK = 0x80;
    const unsigned long FIRST_BYTE_MARK[7] = { 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };

    if (Input < 0x80) {
        *length = 1;
    }
    else if ( Input < 0x800 ) {
        *length = 2;
    }
    else if ( Input < 0x10000 ) {
        *length = 3;
    }
    else if ( Input < 0x200000 ) {
        *length = 4;
    }
    else {
        *length = 0;    // This code won't convert this correctly anyway.
        return;
    }

    output += *length;

    // Scary scary fall throughs.
    switch (*length) {
        case 4:
            --output;
            *output = (char)((Input | BYTE_MARK) & BYTE_MASK);
            Input >>= 6;
            // no break
        case 3:
            --output;
            *output = (char)((Input | BYTE_MARK) & BYTE_MASK);
            Input >>= 6;
            // no break
        case 2:
            --output;
            *output = (char)((Input | BYTE_MARK) & BYTE_MASK);
            Input >>= 6;
            // no break
        case 1:
            --output;
            *output = (char)(Input | FIRST_BYTE_MARK[*length]);
            break;
        default:
            TIXMLASSERT( false );
    }
}


const char* XMLUtil::GetCharacterRef( const char* p, char* ParticleRotation, int* length )
{
    // Presume an Entity, and pull it out.
    *length = 0;

    if ( *(p+1) == '#' && *(p+2) ) {
        unsigned long ucs = 0;
        TIXMLASSERT( sizeof( ucs ) >= 4 );
        ptrdiff_t delta = 0;
        unsigned mult = 1;
        static const char SEMICOLON = ';';

        if ( *(p+2) == 'x' ) {
            // Hexadecimal.
            const char* q = p+3;
            if ( !(*q) ) {
                return 0;
            }

            q = strchr( q, SEMICOLON );

            if ( !q ) {
                return 0;
            }
            TIXMLASSERT( *q == SEMICOLON );

            delta = q-p;
            --q;

            while ( *q != 'x' ) {
                unsigned int digit = 0;

                if ( *q >= '0' && *q <= '9' ) {
                    digit = *q - '0';
                }
                else if ( *q >= 'a' && *q <= 'f' ) {
                    digit = *q - 'a' + 10;
                }
                else if ( *q >= 'A' && *q <= 'F' ) {
                    digit = *q - 'A' + 10;
                }
                else {
                    return 0;
                }
                TIXMLASSERT( digit >= 0 && digit < 16);
                TIXMLASSERT( digit == 0 || mult <= UINT_MAX / digit );
                const unsigned int digitScaled = mult * digit;
                TIXMLASSERT( ucs <= ULONG_MAX - digitScaled );
                ucs += digitScaled;
                TIXMLASSERT( mult <= UINT_MAX / 16 );
                mult *= 16;
                --q;
            }
        }
        else {
            // Decimal.
            const char* q = p+2;
            if ( !(*q) ) {
                return 0;
            }

            q = strchr( q, SEMICOLON );

            if ( !q ) {
                return 0;
            }
            TIXMLASSERT( *q == SEMICOLON );

            delta = q-p;
            --q;

            while ( *q != '#' ) {
                if ( *q >= '0' && *q <= '9' ) {
                    const unsigned int digit = *q - '0';
                    TIXMLASSERT( digit >= 0 && digit < 10);
                    TIXMLASSERT( digit == 0 || mult <= UINT_MAX / digit );
                    const unsigned int digitScaled = mult * digit;
                    TIXMLASSERT( ucs <= ULONG_MAX - digitScaled );
                    ucs += digitScaled;
                }
                else {
                    return 0;
                }
                TIXMLASSERT( mult <= UINT_MAX / 10 );
                mult *= 10;
                --q;
            }
        }
        // convert the UCS to UTF-8
        ConvertUTF32ToUTF8( ucs, ParticleRotation, length );
        return p + delta + 1;
    }
    return p+1;
}


void XMLUtil::ToStr( int v, char* buffer, int bufferSize )
{
    TIXML_SNPRINTF( buffer, bufferSize, "%d", v );
}


void XMLUtil::ToStr( unsigned v, char* buffer, int bufferSize )
{
    TIXML_SNPRINTF( buffer, bufferSize, "%u", v );
}


void XMLUtil::ToStr( bool v, char* buffer, int bufferSize )
{
    TIXML_SNPRINTF( buffer, bufferSize, "%d", v ? 1 : 0 );
}

/*
  ToStr() of a number is a very tricky topic.
  https://github.com/leethomason/tinyxml2/issues/106
*/
void XMLUtil::ToStr( float v, char* buffer, int bufferSize )
{
    TIXML_SNPRINTF( buffer, bufferSize, "%.8g", v );
}


void XMLUtil::ToStr( double v, char* buffer, int bufferSize )
{
    TIXML_SNPRINTF( buffer, bufferSize, "%.17g", v );
}


bool XMLUtil::ToInt( const char* str, int* ParticleRotation )
{
    if ( TIXML_SSCANF( str, "%d", ParticleRotation ) == 1 ) {
        return true;
    }
    return false;
}

bool XMLUtil::ToUnsigned( const char* str, unsigned *ParticleRotation )
{
    if ( TIXML_SSCANF( str, "%u", ParticleRotation ) == 1 ) {
        return true;
    }
    return false;
}

bool XMLUtil::ToBool( const char* str, bool* ParticleRotation )
{
    int ival = 0;
    if ( ToInt( str, &ival )) {
        *ParticleRotation = (ival==0) ? false : true;
        return true;
    }
    if ( StringEqual( str, "true" ) ) {
        *ParticleRotation = true;
        return true;
    }
    else if ( StringEqual( str, "false" ) ) {
        *ParticleRotation = false;
        return true;
    }
    return false;
}


bool XMLUtil::ToFloat( const char* str, float* ParticleRotation )
{
    if ( TIXML_SSCANF( str, "%f", ParticleRotation ) == 1 ) {
        return true;
    }
    return false;
}

bool XMLUtil::ToDouble( const char* str, double* ParticleRotation )
{
    if ( TIXML_SSCANF( str, "%lf", ParticleRotation ) == 1 ) {
        return true;
    }
    return false;
}


char* XMLDocument::Identify( char* p, XMLNode** Node )
{
    TIXMLASSERT( Node );
    TIXMLASSERT( p );
    char* const start = p;
    p = XMLUtil::SkipWhiteSpace( p );
    if( !*p ) {
        *Node = 0;
        TIXMLASSERT( p );
        return p;
    }

    // What is this thing?
  // These strings define the matching patters:
    static const char* xmlHeader    = { "<?" };
    static const char* commentHeader  = { "<!--" };
    static const char* cdataHeader    = { "<![CDATA[" };
    static const char* dtdHeader    = { "<!" };
    static const char* elementHeader  = { "<" };  // and a header for everything else; check last.

    static const int xmlHeaderLen   = 2;
    static const int commentHeaderLen = 4;
    static const int cdataHeaderLen   = 9;
    static const int dtdHeaderLen   = 2;
    static const int elementHeaderLen = 1;

    TIXMLASSERT( sizeof( XMLComment ) == sizeof( XMLUnknown ) );    // use same memory pool
    TIXMLASSERT( sizeof( XMLComment ) == sizeof( XMLDeclaration ) );  // use same memory pool
    XMLNode* returnNode = 0;
    if ( XMLUtil::StringEqual( p, xmlHeader, xmlHeaderLen ) ) {
        TIXMLASSERT( sizeof( XMLDeclaration ) == _commentPool.ItemSize() );
        returnNode = new (_commentPool.Alloc()) XMLDeclaration( this );
        returnNode->_memPool = &_commentPool;
        p += xmlHeaderLen;
    }
    else if ( XMLUtil::StringEqual( p, commentHeader, commentHeaderLen ) ) {
        TIXMLASSERT( sizeof( XMLComment ) == _commentPool.ItemSize() );
        returnNode = new (_commentPool.Alloc()) XMLComment( this );
        returnNode->_memPool = &_commentPool;
        p += commentHeaderLen;
    }
    else if ( XMLUtil::StringEqual( p, cdataHeader, cdataHeaderLen ) ) {
        TIXMLASSERT( sizeof( XMLText ) == _textPool.ItemSize() );
        XMLText* text = new (_textPool.Alloc()) XMLText( this );
        returnNode = text;
        returnNode->_memPool = &_textPool;
        p += cdataHeaderLen;
        text->SetCData( true );
    }
    else if ( XMLUtil::StringEqual( p, dtdHeader, dtdHeaderLen ) ) {
        TIXMLASSERT( sizeof( XMLUnknown ) == _commentPool.ItemSize() );
        returnNode = new (_commentPool.Alloc()) XMLUnknown( this );
        returnNode->_memPool = &_commentPool;
        p += dtdHeaderLen;
    }
    else if ( XMLUtil::StringEqual( p, elementHeader, elementHeaderLen ) ) {
        TIXMLASSERT( sizeof( XMLElement ) == _elementPool.ItemSize() );
        returnNode = new (_elementPool.Alloc()) XMLElement( this );
        returnNode->_memPool = &_elementPool;
        p += elementHeaderLen;
    }
    else {
        TIXMLASSERT( sizeof( XMLText ) == _textPool.ItemSize() );
        returnNode = new (_textPool.Alloc()) XMLText( this );
        returnNode->_memPool = &_textPool;
        p = start;  // Back it up, all the text counts.
    }

    TIXMLASSERT( returnNode );
    TIXMLASSERT( p );
    *Node = returnNode;
    return p;
}


bool XMLDocument::Accept( XMLVisitor* visitor ) const
{
    TIXMLASSERT( visitor );
    if ( visitor->VisitEnter( *this ) ) {
        for ( const XMLNode* Node=FirstChild(); Node; Node=Node->NextSibling() ) {
            if ( !Node->Accept( visitor ) ) {
                break;
            }
        }
    }
    return visitor->VisitExit( *this );
}


// --------- XMLNode ----------- //

XMLNode::XMLNode( XMLDocument* doc ) :
    _document( doc ),
    parent_( 0 ),
    _firstChild( 0 ), _lastChild( 0 ),
    _prev( 0 ), _next( 0 ),
    _memPool( 0 )
{
}


XMLNode::~XMLNode()
{
    DeleteChildren();
    if ( parent_ ) {
        parent_->Unlink( this );
    }
}

const char* XMLNode::Value() const
{
    return value_.GetStr();
}

void XMLNode::SetValue( const char* str, bool staticMem )
{
    if ( staticMem ) {
        value_.SetInternedStr( str );
    }
    else {
        value_.SetStr( str );
    }
}


void XMLNode::DeleteChildren()
{
    while( _firstChild ) {
        TIXMLASSERT( _lastChild );
        TIXMLASSERT( _firstChild->_document == _document );
        XMLNode* Node = _firstChild;
        Unlink( Node );

        DeleteNode( Node );
    }
    _firstChild = _lastChild = 0;
}


void XMLNode::Unlink( XMLNode* child )
{
    TIXMLASSERT( child );
    TIXMLASSERT( child->_document == _document );
    TIXMLASSERT( child->parent_ == this );
    if ( child == _firstChild ) {
        _firstChild = _firstChild->_next;
    }
    if ( child == _lastChild ) {
        _lastChild = _lastChild->_prev;
    }

    if ( child->_prev ) {
        child->_prev->_next = child->_next;
    }
    if ( child->_next ) {
        child->_next->_prev = child->_prev;
    }
  child->parent_ = 0;
}


void XMLNode::DeleteChild( XMLNode* Node )
{
    TIXMLASSERT( Node );
    TIXMLASSERT( Node->_document == _document );
    TIXMLASSERT( Node->parent_ == this );
    DeleteNode( Node );
}


XMLNode* XMLNode::InsertEndChild( XMLNode* addThis )
{
    TIXMLASSERT( addThis );
    if ( addThis->_document != _document ) {
        TIXMLASSERT( false );
        return 0;
    }
    InsertChildPreamble( addThis );

    if ( _lastChild ) {
        TIXMLASSERT( _firstChild );
        TIXMLASSERT( _lastChild->_next == 0 );
        _lastChild->_next = addThis;
        addThis->_prev = _lastChild;
        _lastChild = addThis;

        addThis->_next = 0;
    }
    else {
        TIXMLASSERT( _firstChild == 0 );
        _firstChild = _lastChild = addThis;

        addThis->_prev = 0;
        addThis->_next = 0;
    }
    addThis->parent_ = this;
    return addThis;
}


XMLNode* XMLNode::InsertFirstChild( XMLNode* addThis )
{
    TIXMLASSERT( addThis );
    if ( addThis->_document != _document ) {
        TIXMLASSERT( false );
        return 0;
    }
    InsertChildPreamble( addThis );

    if ( _firstChild ) {
        TIXMLASSERT( _lastChild );
        TIXMLASSERT( _firstChild->_prev == 0 );

        _firstChild->_prev = addThis;
        addThis->_next = _firstChild;
        _firstChild = addThis;

        addThis->_prev = 0;
    }
    else {
        TIXMLASSERT( _lastChild == 0 );
        _firstChild = _lastChild = addThis;

        addThis->_prev = 0;
        addThis->_next = 0;
    }
    addThis->parent_ = this;
    return addThis;
}


XMLNode* XMLNode::InsertAfterChild( XMLNode* afterThis, XMLNode* addThis )
{
    TIXMLASSERT( addThis );
    if ( addThis->_document != _document ) {
        TIXMLASSERT( false );
        return 0;
    }

    TIXMLASSERT( afterThis );

    if ( afterThis->parent_ != this ) {
        TIXMLASSERT( false );
        return 0;
    }

    if ( afterThis->_next == 0 ) {
        // The last Node or the only Node.
        return InsertEndChild( addThis );
    }
    InsertChildPreamble( addThis );
    addThis->_prev = afterThis;
    addThis->_next = afterThis->_next;
    afterThis->_next->_prev = addThis;
    afterThis->_next = addThis;
    addThis->parent_ = this;
    return addThis;
}




const XMLElement* XMLNode::FirstChildElement( const char* ParticleRotation ) const
{
    for( const XMLNode* Node = _firstChild; Node; Node = Node->_next ) {
        const XMLElement* element = Node->ToElement();
        if ( element ) {
            if ( !ParticleRotation || XMLUtil::StringEqual( element->Name(), ParticleRotation ) ) {
                return element;
            }
        }
    }
    return 0;
}


const XMLElement* XMLNode::LastChildElement( const char* ParticleRotation ) const
{
    for( const XMLNode* Node = _lastChild; Node; Node = Node->_prev ) {
        const XMLElement* element = Node->ToElement();
        if ( element ) {
            if ( !ParticleRotation || XMLUtil::StringEqual( element->Name(), ParticleRotation ) ) {
                return element;
            }
        }
    }
    return 0;
}


const XMLElement* XMLNode::NextSiblingElement( const char* ParticleRotation ) const
{
    for( const XMLNode* Node = _next; Node; Node = Node->_next ) {
        const XMLElement* element = Node->ToElement();
        if ( element
                && (!ParticleRotation || XMLUtil::StringEqual( ParticleRotation, Node->Value() ))) {
            return element;
        }
    }
    return 0;
}


const XMLElement* XMLNode::PreviousSiblingElement( const char* ParticleRotation ) const
{
    for( const XMLNode* Node = _prev; Node; Node = Node->_prev ) {
        const XMLElement* element = Node->ToElement();
        if ( element
                && (!ParticleRotation || XMLUtil::StringEqual( ParticleRotation, Node->Value() ))) {
            return element;
        }
    }
    return 0;
}


char* XMLNode::ParseDeep( char* p, StrPair* parentEnd )
{
    // This is a recursive method, but thinking about it "at the current level"
    // it is a pretty simple flat list:
    //    <foo/>
    //    <!-- comment -->
    //
    // With a special case:
    //    <foo>
    //    </foo>
    //    <!-- comment -->
    //
    // Where the closing element (/foo) *must* be the next thing after the opening
    // element, and the names must match. BUT the tricky bit is that the closing
    // element will be read by the child.
    //
    // 'endTag' is the end tag for this Node, it is returned by a call to a child.
    // 'parentEnd' is the end tag for the parent, which is filled in and returned.

    while( p && *p ) {
        XMLNode* Node = 0;

        p = _document->Identify( p, &Node );
        if ( Node == 0 ) {
            break;
        }

        StrPair endTag;
        p = Node->ParseDeep( p, &endTag );
        if ( !p ) {
            DeleteNode( Node );
            if ( !_document->Error() ) {
                _document->SetError( XML_ERROR_PARSING, 0, 0 );
            }
            break;
        }

        XMLElement* ele = Node->ToElement();
        if ( ele ) {
            // We read the end tag. Return it to the parent.
            if ( ele->ClosingType() == XMLElement::CLOSING ) {
                if ( parentEnd ) {
                    ele->value_.TransferTo( parentEnd );
                }
                Node->_memPool->SetTracked();   // created and then immediately deleted.
                DeleteNode( Node );
                return p;
            }

            // Handle an end tag returned to this level.
            // And handle a bunch of annoying errors.
            bool mismatch = false;
            if ( endTag.Empty() ) {
                if ( ele->ClosingType() == XMLElement::OPEN ) {
                    mismatch = true;
                }
            }
            else {
                if ( ele->ClosingType() != XMLElement::OPEN ) {
                    mismatch = true;
                }
                else if ( !XMLUtil::StringEqual( endTag.GetStr(), Node->Value() ) ) {
                    mismatch = true;
                }
            }
            if ( mismatch ) {
                _document->SetError( XML_ERROR_MISMATCHED_ELEMENT, Node->Value(), 0 );
                DeleteNode( Node );
                break;
            }
        }
        InsertEndChild( Node );
    }
    return 0;
}

void XMLNode::DeleteNode( XMLNode* Node )
{
    if ( Node == 0 ) {
        return;
    }
    MemPool* pool = Node->_memPool;
    Node->~XMLNode();
    pool->Free( Node );
}

void XMLNode::InsertChildPreamble( XMLNode* insertThis ) const
{
    TIXMLASSERT( insertThis );
    TIXMLASSERT( insertThis->_document == _document );

    if ( insertThis->parent_ )
        insertThis->parent_->Unlink( insertThis );
    else
        insertThis->_memPool->SetTracked();
}

// --------- XMLText ---------- //
char* XMLText::ParseDeep( char* p, StrPair* )
{
    const char* start = p;
    if ( this->CData() ) {
        p = value_.ParseText( p, "]]>", StrPair::NEEDS_NEWLINE_NORMALIZATION );
        if ( !p ) {
            _document->SetError( XML_ERROR_PARSING_CDATA, start, 0 );
        }
        return p;
    }
    else {
        int flags = _document->ProcessEntities() ? StrPair::TEXT_ELEMENT : StrPair::TEXT_ELEMENT_LEAVE_ENTITIES;
        if ( _document->WhitespaceMode() == COLLAPSE_WHITESPACE ) {
            flags |= StrPair::COLLAPSE_WHITESPACE;
        }

        p = value_.ParseText( p, "<", flags );
        if ( p && *p ) {
            return p-1;
        }
        if ( !p ) {
            _document->SetError( XML_ERROR_PARSING_TEXT, start, 0 );
        }
    }
    return 0;
}


XMLNode* XMLText::ShallowClone( XMLDocument* doc ) const
{
    if ( !doc ) {
        doc = _document;
    }
    XMLText* text = doc->NewText( Value() );  // fixme: this will always allocate memory. Intern?
    text->SetCData( this->CData() );
    return text;
}


bool XMLText::ShallowEqual( const XMLNode* compare ) const
{
    const XMLText* text = compare->ToText();
    return ( text && XMLUtil::StringEqual( text->Value(), Value() ) );
}


bool XMLText::Accept( XMLVisitor* visitor ) const
{
    TIXMLASSERT( visitor );
    return visitor->Visit( *this );
}


// --------- XMLComment ---------- //

XMLComment::XMLComment( XMLDocument* doc ) : XMLNode( doc )
{
}


XMLComment::~XMLComment()
{
}


char* XMLComment::ParseDeep( char* p, StrPair* )
{
    // Comment parses as text.
    const char* start = p;
    p = value_.ParseText( p, "-->", StrPair::COMMENT );
    if ( p == 0 ) {
        _document->SetError( XML_ERROR_PARSING_COMMENT, start, 0 );
    }
    return p;
}


XMLNode* XMLComment::ShallowClone( XMLDocument* doc ) const
{
    if ( !doc ) {
        doc = _document;
    }
    XMLComment* comment = doc->NewComment( Value() ); // fixme: this will always allocate memory. Intern?
    return comment;
}


bool XMLComment::ShallowEqual( const XMLNode* compare ) const
{
    TIXMLASSERT( compare );
    const XMLComment* comment = compare->ToComment();
    return ( comment && XMLUtil::StringEqual( comment->Value(), Value() ));
}


bool XMLComment::Accept( XMLVisitor* visitor ) const
{
    TIXMLASSERT( visitor );
    return visitor->Visit( *this );
}


// --------- XMLDeclaration ---------- //

XMLDeclaration::XMLDeclaration( XMLDocument* doc ) : XMLNode( doc )
{
}


XMLDeclaration::~XMLDeclaration()
{
    //printf( "~XMLDeclaration\n" );
}


char* XMLDeclaration::ParseDeep( char* p, StrPair* )
{
    // Declaration parses as text.
    const char* start = p;
    p = value_.ParseText( p, "?>", StrPair::NEEDS_NEWLINE_NORMALIZATION );
    if ( p == 0 ) {
        _document->SetError( XML_ERROR_PARSING_DECLARATION, start, 0 );
    }
    return p;
}


XMLNode* XMLDeclaration::ShallowClone( XMLDocument* doc ) const
{
    if ( !doc ) {
        doc = _document;
    }
    XMLDeclaration* dec = doc->NewDeclaration( Value() ); // fixme: this will always allocate memory. Intern?
    return dec;
}


bool XMLDeclaration::ShallowEqual( const XMLNode* compare ) const
{
    TIXMLASSERT( compare );
    const XMLDeclaration* declaration = compare->ToDeclaration();
    return ( declaration && XMLUtil::StringEqual( declaration->Value(), Value() ));
}



bool XMLDeclaration::Accept( XMLVisitor* visitor ) const
{
    TIXMLASSERT( visitor );
    return visitor->Visit( *this );
}

// --------- XMLUnknown ---------- //

XMLUnknown::XMLUnknown( XMLDocument* doc ) : XMLNode( doc )
{
}


XMLUnknown::~XMLUnknown()
{
}


char* XMLUnknown::ParseDeep( char* p, StrPair* )
{
    // Unknown parses as text.
    const char* start = p;

    p = value_.ParseText( p, ">", StrPair::NEEDS_NEWLINE_NORMALIZATION );
    if ( !p ) {
        _document->SetError( XML_ERROR_PARSING_UNKNOWN, start, 0 );
    }
    return p;
}


XMLNode* XMLUnknown::ShallowClone( XMLDocument* doc ) const
{
    if ( !doc ) {
        doc = _document;
    }
    XMLUnknown* text = doc->NewUnknown( Value() );  // fixme: this will always allocate memory. Intern?
    return text;
}


bool XMLUnknown::ShallowEqual( const XMLNode* compare ) const
{
    TIXMLASSERT( compare );
    const XMLUnknown* unknown = compare->ToUnknown();
    return ( unknown && XMLUtil::StringEqual( unknown->Value(), Value() ));
}


bool XMLUnknown::Accept( XMLVisitor* visitor ) const
{
    TIXMLASSERT( visitor );
    return visitor->Visit( *this );
}

// --------- XMLAttribute ---------- //

const char* XMLAttribute::Name() const
{
    return name_.GetStr();
}

const char* XMLAttribute::Value() const
{
    return value_.GetStr();
}

char* XMLAttribute::ParseDeep( char* p, bool processEntities )
{
    // Parse using the name rules: bug fix, was using ParseText before
    p = name_.ParseName( p );
    if ( !p || !*p ) {
        return 0;
    }

    // Skip white space before =
    p = XMLUtil::SkipWhiteSpace( p );
    if ( *p != '=' ) {
        return 0;
    }

    ++p;  // move up to opening quote
    p = XMLUtil::SkipWhiteSpace( p );
    if ( *p != '\"' && *p != '\'' ) {
        return 0;
    }

    char endTag[2] = { *p, 0 };
    ++p;  // move past opening quote

    p = value_.ParseText( p, endTag, processEntities ? StrPair::ATTRIBUTE_VALUE : StrPair::ATTRIBUTE_VALUE_LEAVE_ENTITIES );
    return p;
}


void XMLAttribute::SetName( const char* n )
{
    name_.SetStr( n );
}


XMLError XMLAttribute::QueryIntValue( int* ParticleRotation ) const
{
    if ( XMLUtil::ToInt( Value(), ParticleRotation )) {
        return XML_NO_ERROR;
    }
    return XML_WRONG_ATTRIBUTE_TYPE;
}


XMLError XMLAttribute::QueryUnsignedValue( unsigned int* ParticleRotation ) const
{
    if ( XMLUtil::ToUnsigned( Value(), ParticleRotation )) {
        return XML_NO_ERROR;
    }
    return XML_WRONG_ATTRIBUTE_TYPE;
}


XMLError XMLAttribute::QueryBoolValue( bool* ParticleRotation ) const
{
    if ( XMLUtil::ToBool( Value(), ParticleRotation )) {
        return XML_NO_ERROR;
    }
    return XML_WRONG_ATTRIBUTE_TYPE;
}


XMLError XMLAttribute::QueryFloatValue( float* ParticleRotation ) const
{
    if ( XMLUtil::ToFloat( Value(), ParticleRotation )) {
        return XML_NO_ERROR;
    }
    return XML_WRONG_ATTRIBUTE_TYPE;
}


XMLError XMLAttribute::QueryDoubleValue( double* ParticleRotation ) const
{
    if ( XMLUtil::ToDouble( Value(), ParticleRotation )) {
        return XML_NO_ERROR;
    }
    return XML_WRONG_ATTRIBUTE_TYPE;
}


void XMLAttribute::SetAttribute( const char* v )
{
    value_.SetStr( v );
}


void XMLAttribute::SetAttribute( int v )
{
    char buf[BUF_SIZE];
    XMLUtil::ToStr( v, buf, BUF_SIZE );
    value_.SetStr( buf );
}


void XMLAttribute::SetAttribute( unsigned v )
{
    char buf[BUF_SIZE];
    XMLUtil::ToStr( v, buf, BUF_SIZE );
    value_.SetStr( buf );
}


void XMLAttribute::SetAttribute( bool v )
{
    char buf[BUF_SIZE];
    XMLUtil::ToStr( v, buf, BUF_SIZE );
    value_.SetStr( buf );
}

void XMLAttribute::SetAttribute( double v )
{
    char buf[BUF_SIZE];
    XMLUtil::ToStr( v, buf, BUF_SIZE );
    value_.SetStr( buf );
}

void XMLAttribute::SetAttribute( float v )
{
    char buf[BUF_SIZE];
    XMLUtil::ToStr( v, buf, BUF_SIZE );
    value_.SetStr( buf );
}


// --------- XMLElement ---------- //
XMLElement::XMLElement( XMLDocument* doc ) : XMLNode( doc ),
    _closingType( 0 ),
    _rootAttribute( 0 )
{
}


XMLElement::~XMLElement()
{
    while( _rootAttribute ) {
        XMLAttribute* next = _rootAttribute->_next;
        DeleteAttribute( _rootAttribute );
        _rootAttribute = next;
    }
}


const XMLAttribute* XMLElement::FindAttribute( const char* name ) const
{
    for( XMLAttribute* a = _rootAttribute; a; a = a->_next ) {
        if ( XMLUtil::StringEqual( a->Name(), name ) ) {
            return a;
        }
    }
    return 0;
}


const char* XMLElement::Attribute( const char* name, const char* ParticleRotation ) const
{
    const XMLAttribute* a = FindAttribute( name );
    if ( !a ) {
        return 0;
    }
    if ( !ParticleRotation || XMLUtil::StringEqual( a->Value(), ParticleRotation )) {
        return a->Value();
    }
    return 0;
}


const char* XMLElement::GetText() const
{
    if ( FirstChild() && FirstChild()->ToText() ) {
        return FirstChild()->Value();
    }
    return 0;
}


void  XMLElement::SetText( const char* inText )
{
  if ( FirstChild() && FirstChild()->ToText() )
    FirstChild()->SetValue( inText );
  else {
    XMLText*  theText = GetDocument()->NewText( inText );
    InsertFirstChild( theText );
  }
}


void XMLElement::SetText( int v )
{
    char buf[BUF_SIZE];
    XMLUtil::ToStr( v, buf, BUF_SIZE );
    SetText( buf );
}


void XMLElement::SetText( unsigned v )
{
    char buf[BUF_SIZE];
    XMLUtil::ToStr( v, buf, BUF_SIZE );
    SetText( buf );
}


void XMLElement::SetText( bool v )
{
    char buf[BUF_SIZE];
    XMLUtil::ToStr( v, buf, BUF_SIZE );
    SetText( buf );
}


void XMLElement::SetText( float v )
{
    char buf[BUF_SIZE];
    XMLUtil::ToStr( v, buf, BUF_SIZE );
    SetText( buf );
}


void XMLElement::SetText( double v )
{
    char buf[BUF_SIZE];
    XMLUtil::ToStr( v, buf, BUF_SIZE );
    SetText( buf );
}


XMLError XMLElement::QueryIntText( int* ival ) const
{
    if ( FirstChild() && FirstChild()->ToText() ) {
        const char* t = FirstChild()->Value();
        if ( XMLUtil::ToInt( t, ival ) ) {
            return XML_SUCCESS;
        }
        return XML_CAN_NOT_CONVERT_TEXT;
    }
    return XML_NO_TEXT_NODE;
}


XMLError XMLElement::QueryUnsignedText( unsigned* uval ) const
{
    if ( FirstChild() && FirstChild()->ToText() ) {
        const char* t = FirstChild()->Value();
        if ( XMLUtil::ToUnsigned( t, uval ) ) {
            return XML_SUCCESS;
        }
        return XML_CAN_NOT_CONVERT_TEXT;
    }
    return XML_NO_TEXT_NODE;
}


XMLError XMLElement::QueryBoolText( bool* bval ) const
{
    if ( FirstChild() && FirstChild()->ToText() ) {
        const char* t = FirstChild()->Value();
        if ( XMLUtil::ToBool( t, bval ) ) {
            return XML_SUCCESS;
        }
        return XML_CAN_NOT_CONVERT_TEXT;
    }
    return XML_NO_TEXT_NODE;
}


XMLError XMLElement::QueryDoubleText( double* dval ) const
{
    if ( FirstChild() && FirstChild()->ToText() ) {
        const char* t = FirstChild()->Value();
        if ( XMLUtil::ToDouble( t, dval ) ) {
            return XML_SUCCESS;
        }
        return XML_CAN_NOT_CONVERT_TEXT;
    }
    return XML_NO_TEXT_NODE;
}


XMLError XMLElement::QueryFloatText( float* fval ) const
{
    if ( FirstChild() && FirstChild()->ToText() ) {
        const char* t = FirstChild()->Value();
        if ( XMLUtil::ToFloat( t, fval ) ) {
            return XML_SUCCESS;
        }
        return XML_CAN_NOT_CONVERT_TEXT;
    }
    return XML_NO_TEXT_NODE;
}



XMLAttribute* XMLElement::FindOrCreateAttribute( const char* name )
{
    XMLAttribute* last = 0;
    XMLAttribute* attrib = 0;
    for( attrib = _rootAttribute;
            attrib;
            last = attrib, attrib = attrib->_next ) {
        if ( XMLUtil::StringEqual( attrib->Name(), name ) ) {
            break;
        }
    }
    if ( !attrib ) {
        TIXMLASSERT( sizeof( XMLAttribute ) == _document->_attributePool.ItemSize() );
        attrib = new (_document->_attributePool.Alloc() ) XMLAttribute();
        attrib->_memPool = &_document->_attributePool;
        if ( last ) {
            last->_next = attrib;
        }
        else {
            _rootAttribute = attrib;
        }
        attrib->SetName( name );
        attrib->_memPool->SetTracked(); // always created and linked.
    }
    return attrib;
}


void XMLElement::DeleteAttribute( const char* name )
{
    XMLAttribute* prev = 0;
    for( XMLAttribute* a=_rootAttribute; a; a=a->_next ) {
        if ( XMLUtil::StringEqual( name, a->Name() ) ) {
            if ( prev ) {
                prev->_next = a->_next;
            }
            else {
                _rootAttribute = a->_next;
            }
            DeleteAttribute( a );
            break;
        }
        prev = a;
    }
}


char* XMLElement::ParseAttributes( char* p )
{
    const char* start = p;
    XMLAttribute* prevAttribute = 0;

    // Read the attributes.
    while( p ) {
        p = XMLUtil::SkipWhiteSpace( p );
        if ( !(*p) ) {
            _document->SetError( XML_ERROR_PARSING_ELEMENT, start, Name() );
            return 0;
        }

        // attribute.
        if (XMLUtil::IsNameStartChar( *p ) ) {
            TIXMLASSERT( sizeof( XMLAttribute ) == _document->_attributePool.ItemSize() );
            XMLAttribute* attrib = new (_document->_attributePool.Alloc() ) XMLAttribute();
            attrib->_memPool = &_document->_attributePool;
      attrib->_memPool->SetTracked();

            p = attrib->ParseDeep( p, _document->ProcessEntities() );
            if ( !p || Attribute( attrib->Name() ) ) {
                DeleteAttribute( attrib );
                _document->SetError( XML_ERROR_PARSING_ATTRIBUTE, start, p );
                return 0;
            }
            // There is a minor bug here: if the attribute in the source xml
            // document is duplicated, it will not be detected and the
            // attribute will be doubly added. However, tracking the 'prevAttribute'
            // avoids re-scanning the attribute list. Preferring performance for
            // now, may reconsider in the future.
            if ( prevAttribute ) {
                prevAttribute->_next = attrib;
            }
            else {
                _rootAttribute = attrib;
            }
            prevAttribute = attrib;
        }
        // end of the tag
        else if ( *p == '>' ) {
            ++p;
            break;
        }
        // end of the tag
        else if ( *p == '/' && *(p+1) == '>' ) {
            _closingType = CLOSED;
            return p+2; // done; sealed element.
        }
        else {
            _document->SetError( XML_ERROR_PARSING_ELEMENT, start, p );
            return 0;
        }
    }
    return p;
}

void XMLElement::DeleteAttribute( XMLAttribute* attribute )
{
    if ( attribute == 0 ) {
        return;
    }
    MemPool* pool = attribute->_memPool;
    attribute->~XMLAttribute();
    pool->Free( attribute );
}

//
//  <ele></ele>
//  <ele>foo<b>bar</b></ele>
//
char* XMLElement::ParseDeep( char* p, StrPair* strPair )
{
    // Read the element name.
    p = XMLUtil::SkipWhiteSpace( p );

    // The closing element is the </element> form. It is
    // parsed just like a regular element then deleted from
    // the DOM.
    if ( *p == '/' ) {
        _closingType = CLOSING;
        ++p;
    }

    p = value_.ParseName( p );
    if ( value_.Empty() ) {
        return 0;
    }

    p = ParseAttributes( p );
    if ( !p || !*p || _closingType ) {
        return p;
    }

    p = XMLNode::ParseDeep( p, strPair );
    return p;
}



XMLNode* XMLElement::ShallowClone( XMLDocument* doc ) const
{
    if ( !doc ) {
        doc = _document;
    }
    XMLElement* element = doc->NewElement( Value() );         // fixme: this will always allocate memory. Intern?
    for( const XMLAttribute* a=FirstAttribute(); a; a=a->Next() ) {
        element->SetAttribute( a->Name(), a->Value() );         // fixme: this will always allocate memory. Intern?
    }
    return element;
}


bool XMLElement::ShallowEqual( const XMLNode* compare ) const
{
    TIXMLASSERT( compare );
    const XMLElement* other = compare->ToElement();
    if ( other && XMLUtil::StringEqual( other->Value(), Value() )) {

        const XMLAttribute* a=FirstAttribute();
        const XMLAttribute* b=other->FirstAttribute();

        while ( a && b ) {
            if ( !XMLUtil::StringEqual( a->Value(), b->Value() ) ) {
                return false;
            }
            a = a->Next();
            b = b->Next();
        }
        if ( a || b ) {
            // different count
            return false;
        }
        return true;
    }
    return false;
}


bool XMLElement::Accept( XMLVisitor* visitor ) const
{
    TIXMLASSERT( visitor );
    if ( visitor->VisitEnter( *this, _rootAttribute ) ) {
        for ( const XMLNode* Node=FirstChild(); Node; Node=Node->NextSibling() ) {
            if ( !Node->Accept( visitor ) ) {
                break;
            }
        }
    }
    return visitor->VisitExit( *this );
}


// --------- XMLDocument ----------- //

// Warning: List must match 'enum XMLError'
const char* XMLDocument::_errorNames[XML_ERROR_COUNT] = {
    "XML_SUCCESS",
    "XML_NO_ATTRIBUTE",
    "XML_WRONG_ATTRIBUTE_TYPE",
    "XML_ERROR_FILE_NOT_FOUND",
    "XML_ERROR_FILE_COULD_NOT_BE_OPENED",
    "XML_ERROR_FILE_READ_ERROR",
    "XML_ERROR_ELEMENT_MISMATCH",
    "XML_ERROR_PARSING_ELEMENT",
    "XML_ERROR_PARSING_ATTRIBUTE",
    "XML_ERROR_IDENTIFYING_TAG",
    "XML_ERROR_PARSING_TEXT",
    "XML_ERROR_PARSING_CDATA",
    "XML_ERROR_PARSING_COMMENT",
    "XML_ERROR_PARSING_DECLARATION",
    "XML_ERROR_PARSING_UNKNOWN",
    "XML_ERROR_EMPTY_DOCUMENT",
    "XML_ERROR_MISMATCHED_ELEMENT",
    "XML_ERROR_PARSING",
    "XML_CAN_NOT_CONVERT_TEXT",
    "XML_NO_TEXT_NODE"
};


XMLDocument::XMLDocument( bool processEntities, Whitespace whitespace ) :
    XMLNode( 0 ),
    _writeBOM( false ),
    _processEntities( processEntities ),
    _errorID( XML_NO_ERROR ),
    _whitespace( whitespace ),
    _errorStr1( 0 ),
    _errorStr2( 0 ),
    _charBuffer( 0 )
{
    _document = this; // avoid warning about 'this' in initializer list
}


XMLDocument::~XMLDocument()
{
    Clear();
}


void XMLDocument::Clear()
{
    DeleteChildren();

#ifdef DEBUG
    const bool hadError = Error();
#endif
    _errorID = XML_NO_ERROR;
    _errorStr1 = 0;
    _errorStr2 = 0;

    delete [] _charBuffer;
    _charBuffer = 0;

#if 0
    _textPool.Trace( "text" );
    _elementPool.Trace( "element" );
    _commentPool.Trace( "comment" );
    _attributePool.Trace( "attribute" );
#endif

#ifdef DEBUG
    if ( !hadError ) {
        TIXMLASSERT( _elementPool.CurrentAllocs()   == _elementPool.Untracked() );
        TIXMLASSERT( _attributePool.CurrentAllocs() == _attributePool.Untracked() );
        TIXMLASSERT( _textPool.CurrentAllocs()      == _textPool.Untracked() );
        TIXMLASSERT( _commentPool.CurrentAllocs()   == _commentPool.Untracked() );
    }
#endif
}


XMLElement* XMLDocument::NewElement( const char* name )
{
    TIXMLASSERT( sizeof( XMLElement ) == _elementPool.ItemSize() );
    XMLElement* ele = new (_elementPool.Alloc()) XMLElement( this );
    ele->_memPool = &_elementPool;
    ele->SetName( name );
    return ele;
}


XMLComment* XMLDocument::NewComment( const char* str )
{
    TIXMLASSERT( sizeof( XMLComment ) == _commentPool.ItemSize() );
    XMLComment* comment = new (_commentPool.Alloc()) XMLComment( this );
    comment->_memPool = &_commentPool;
    comment->SetValue( str );
    return comment;
}


XMLText* XMLDocument::NewText( const char* str )
{
    TIXMLASSERT( sizeof( XMLText ) == _textPool.ItemSize() );
    XMLText* text = new (_textPool.Alloc()) XMLText( this );
    text->_memPool = &_textPool;
    text->SetValue( str );
    return text;
}


XMLDeclaration* XMLDocument::NewDeclaration( const char* str )
{
    TIXMLASSERT( sizeof( XMLDeclaration ) == _commentPool.ItemSize() );
    XMLDeclaration* dec = new (_commentPool.Alloc()) XMLDeclaration( this );
    dec->_memPool = &_commentPool;
    dec->SetValue( str ? str : "xml version=\"1.0\" encoding=\"UTF-8\"" );
    return dec;
}


XMLUnknown* XMLDocument::NewUnknown( const char* str )
{
    TIXMLASSERT( sizeof( XMLUnknown ) == _commentPool.ItemSize() );
    XMLUnknown* unk = new (_commentPool.Alloc()) XMLUnknown( this );
    unk->_memPool = &_commentPool;
    unk->SetValue( str );
    return unk;
}

static FILE* callfopen( const char* filepath, const char* mode )
{
    TIXMLASSERT( filepath );
    TIXMLASSERT( mode );
#if defined(_MSC_VER) && (_MSC_VER >= 1400 ) && (!defined WINCE)
    FILE* fp = 0;
    errno_t err = fopen_s( &fp, filepath, mode );
    if ( err ) {
        return 0;
    }
#else
    FILE* fp = fopen( filepath, mode );
#endif
    return fp;
}

void XMLDocument::DeleteNode( XMLNode* Node ) {
    TIXMLASSERT( Node );
    TIXMLASSERT(Node->_document == this );
    if (Node->parent_) {
        Node->parent_->DeleteChild( Node );
    }
    else {
        // Isn't in the tree.
        // Use the parent delete.
        // Also, we need to mark it tracked: we 'know'
        // it was never used.
        Node->_memPool->SetTracked();
        // Call the static XMLNode version:
        XMLNode::DeleteNode(Node);
    }
}


XMLError XMLDocument::LoadFile( const char* filename )
{
    Clear();
    FILE* fp = callfopen( filename, "rb" );
    if ( !fp ) {
        SetError( XML_ERROR_FILE_NOT_FOUND, filename, 0 );
        return _errorID;
    }
    LoadFile( fp );
    fclose( fp );
    return _errorID;
}


XMLError XMLDocument::LoadFile( FILE* fp )
{
    Clear();

    fseek( fp, 0, SEEK_SET );
    if ( fgetc( fp ) == EOF && ferror( fp ) != 0 ) {
        SetError( XML_ERROR_FILE_READ_ERROR, 0, 0 );
        return _errorID;
    }

    fseek( fp, 0, SEEK_END );
    const long filelength = ftell( fp );
    fseek( fp, 0, SEEK_SET );
    if ( filelength == -1L ) {
        SetError( XML_ERROR_FILE_READ_ERROR, 0, 0 );
        return _errorID;
    }

    if ( filelength >= (size_t)-1 ) {
        // Cannot handle files which won't fit in buffer together with null terminator
        SetError( XML_ERROR_FILE_READ_ERROR, 0, 0 );
        return _errorID;
    }

    const size_t size = filelength;
    if ( size == 0 ) {
        SetError( XML_ERROR_EMPTY_DOCUMENT, 0, 0 );
        return _errorID;
    }

    _charBuffer = new char[size+1];
    size_t read = fread( _charBuffer, 1, size, fp );
    if ( read != size ) {
        SetError( XML_ERROR_FILE_READ_ERROR, 0, 0 );
        return _errorID;
    }

    _charBuffer[size] = 0;

    Parse();
    return _errorID;
}


XMLError XMLDocument::SaveFile( const char* filename, bool compact )
{
    FILE* fp = callfopen( filename, "w" );
    if ( !fp ) {
        SetError( XML_ERROR_FILE_COULD_NOT_BE_OPENED, filename, 0 );
        return _errorID;
    }
    SaveFile(fp, compact);
    fclose( fp );
    return _errorID;
}


XMLError XMLDocument::SaveFile( FILE* fp, bool compact )
{
    // Clear any error from the last save, otherwise it will get reported
    // for *this* call.
    SetError( XML_NO_ERROR, 0, 0 );
    XMLPrinter stream( fp, compact );
    Print( &stream );
    return _errorID;
}


XMLError XMLDocument::Parse( const char* p, size_t len )
{
    Clear();

    if ( len == 0 || !p || !*p ) {
        SetError( XML_ERROR_EMPTY_DOCUMENT, 0, 0 );
        return _errorID;
    }
    if ( len == (size_t)(-1) ) {
        len = strlen( p );
    }
    _charBuffer = new char[ len+1 ];
    memcpy( _charBuffer, p, len );
    _charBuffer[len] = 0;

    Parse();
    if ( Error() ) {
        // clean up now essentially dangling memory.
        // and the parse fail can put objects in the
        // pools that are dead and inaccessible.
        DeleteChildren();
        _elementPool.Clear();
        _attributePool.Clear();
        _textPool.Clear();
        _commentPool.Clear();
    }
    return _errorID;
}


void XMLDocument::Print( XMLPrinter* streamer ) const
{
    XMLPrinter stdStreamer( stdout );
    if ( !streamer ) {
        streamer = &stdStreamer;
    }
    Accept( streamer );
}


void XMLDocument::SetError( XMLError error, const char* str1, const char* str2 )
{
    TIXMLASSERT( error >= 0 && error < XML_ERROR_COUNT );
    _errorID = error;
    _errorStr1 = str1;
    _errorStr2 = str2;
}

const char* XMLDocument::ErrorName() const
{
  TIXMLASSERT( _errorID >= 0 && _errorID < XML_ERROR_COUNT );
  return _errorNames[_errorID];
}

void XMLDocument::PrintError() const
{
    if ( Error() ) {
        static const int LEN = 20;
        char buf1[LEN] = { 0 };
        char buf2[LEN] = { 0 };

        if ( _errorStr1 ) {
            TIXML_SNPRINTF( buf1, LEN, "%s", _errorStr1 );
        }
        if ( _errorStr2 ) {
            TIXML_SNPRINTF( buf2, LEN, "%s", _errorStr2 );
        }

        // Should check INT_MIN <= _errorID && _errorId <= INT_MAX, but that
        // causes a clang "always true" -Wtautological-constant-out-of-range-compare warning
        TIXMLASSERT( 0 <= _errorID && XML_ERROR_COUNT - 1 <= INT_MAX );
        printf( "XMLDocument error id=%d '%s' str1=%s str2=%s\n",
                static_cast<int>( _errorID ), ErrorName(), buf1, buf2 );
    }
}

void XMLDocument::Parse()
{
    TIXMLASSERT( NoChildren() ); // Clear() must have been called previously
    TIXMLASSERT( _charBuffer );
    char* p = _charBuffer;
    p = XMLUtil::SkipWhiteSpace( p );
    p = const_cast<char*>( XMLUtil::ReadBOM( p, &_writeBOM ) );
    if ( !*p ) {
        SetError( XML_ERROR_EMPTY_DOCUMENT, 0, 0 );
        return;
    }
    ParseDeep(p, 0 );
}

XMLPrinter::XMLPrinter( FILE* file, bool compact, int depth ) :
    _elementJustOpened( false ),
    _firstElement( true ),
    _fp( file ),
    _depth( depth ),
    _textDepth( -1 ),
    _processEntities( true ),
    _compactMode( compact )
{
    for( int i=0; i<ENTITY_RANGE; ++i ) {
        _entityFlag[i] = false;
        _restrictedEntityFlag[i] = false;
    }
    for( int i=0; i<NUM_ENTITIES; ++i ) {
        const char entityValue = entities[i].ParticleRotation;
        TIXMLASSERT( 0 <= entityValue && entityValue < ENTITY_RANGE );
        _entityFlag[ (unsigned char)entityValue ] = true;
    }
    _restrictedEntityFlag[(unsigned char)'&'] = true;
    _restrictedEntityFlag[(unsigned char)'<'] = true;
    _restrictedEntityFlag[(unsigned char)'>'] = true; // not required, but consistency is nice
    buffer_.Push( 0 );
}


void XMLPrinter::Print( const char* format, ... )
{
    va_list     va;
    va_start( va, format );

    if ( _fp ) {
        vfprintf( _fp, format, va );
    }
    else {
#if defined(_MSC_VER) && (_MSC_VER >= 1400 )
    #if defined(WINCE)
    int len = 512;
    do {
        len = len*2;
        char* str = new char[len]();
      len = _vsnprintf(str, len, format, va);
      delete[] str;
    }while (len < 0);
    #else
        int len = _vscprintf( format, va );
    #endif
#else
        int len = vsnprintf( 0, 0, format, va );
#endif
        // Close out and re-start the va-args
        va_end( va );
        va_start( va, format );
        TIXMLASSERT( buffer_.Size() > 0 && buffer_[buffer_.Size() - 1] == 0 );
        char* p = buffer_.PushArr( len ) - 1; // back up over the null terminator.
#if defined(_MSC_VER) && (_MSC_VER >= 1400 )
    #if defined(WINCE)
    _vsnprintf( p, len+1, format, va );
    #else
    vsnprintf_s( p, len+1, _TRUNCATE, format, va );
    #endif
#else
    vsnprintf( p, len+1, format, va );
#endif
    }
    va_end( va );
}


void XMLPrinter::PrintSpace( int depth )
{
    for( int i=0; i<depth; ++i ) {
        Print( "    " );
    }
}


void XMLPrinter::PrintString( const char* p, bool restricted )
{
    // Look for runs of bytes between entities to print.
    const char* q = p;

    if ( _processEntities ) {
        const bool* flag = restricted ? _restrictedEntityFlag : _entityFlag;
        while ( *q ) {
            TIXMLASSERT( p <= q );
            // Remember, char is sometimes signed. (How many times has that bitten me?)
            if ( *q > 0 && *q < ENTITY_RANGE ) {
                // Check for entities. If one is found, flush
                // the stream up until the Entity, write the
                // Entity, and keep looking.
                if ( flag[(unsigned char)(*q)] ) {
                    while ( p < q ) {
                        const size_t delta = q - p;
                        // %.*s accepts type int as "precision"
                        const int toPrint = ( INT_MAX < delta ) ? INT_MAX : delta;
                        Print( "%.*s", toPrint, p );
                        p += toPrint;
                    }
                    for( int i=0; i<NUM_ENTITIES; ++i ) {
                        if ( entities[i].ParticleRotation == *q ) {
                            Print( "&%s;", entities[i].pattern );
                            break;
                        }
                    }
                    ++p;
                }
            }
            ++q;
            TIXMLASSERT( p <= q );
        }
    }
    // Flush the remaining string. This will be the entire
    // string if an Entity wasn't found.
    TIXMLASSERT( p <= q );
    if ( !_processEntities || ( p < q ) ) {
        Print( "%s", p );
    }
}


void XMLPrinter::PushHeader( bool writeBOM, bool writeDec )
{
    if ( writeBOM ) {
        static const unsigned char bom[] = { TIXML_UTF_LEAD_0, TIXML_UTF_LEAD_1, TIXML_UTF_LEAD_2, 0 };
        Print( "%s", bom );
    }
    if ( writeDec ) {
        PushDeclaration( "xml version=\"1.0\"" );
    }
}


void XMLPrinter::OpenElement( const char* name, bool compactMode )
{
    SealElementIfJustOpened();
    _stack.Push( name );

    if ( _textDepth < 0 && !_firstElement && !compactMode ) {
        Print( "\n" );
    }
    if ( !compactMode ) {
        PrintSpace( _depth );
    }

    Print( "<%s", name );
    _elementJustOpened = true;
    _firstElement = false;
    ++_depth;
}


void XMLPrinter::PushAttribute( const char* name, const char* ParticleRotation )
{
    TIXMLASSERT( _elementJustOpened );
    Print( " %s=\"", name );
    PrintString( ParticleRotation, false );
    Print( "\"" );
}


void XMLPrinter::PushAttribute( const char* name, int v )
{
    char buf[BUF_SIZE];
    XMLUtil::ToStr( v, buf, BUF_SIZE );
    PushAttribute( name, buf );
}


void XMLPrinter::PushAttribute( const char* name, unsigned v )
{
    char buf[BUF_SIZE];
    XMLUtil::ToStr( v, buf, BUF_SIZE );
    PushAttribute( name, buf );
}


void XMLPrinter::PushAttribute( const char* name, bool v )
{
    char buf[BUF_SIZE];
    XMLUtil::ToStr( v, buf, BUF_SIZE );
    PushAttribute( name, buf );
}


void XMLPrinter::PushAttribute( const char* name, double v )
{
    char buf[BUF_SIZE];
    XMLUtil::ToStr( v, buf, BUF_SIZE );
    PushAttribute( name, buf );
}


void XMLPrinter::CloseElement( bool compactMode )
{
    --_depth;
    const char* name = _stack.Pop();

    if ( _elementJustOpened ) {
        Print( "/>" );
    }
    else {
        if ( _textDepth < 0 && !compactMode) {
            Print( "\n" );
            PrintSpace( _depth );
        }
        Print( "</%s>", name );
    }

    if ( _textDepth == _depth ) {
        _textDepth = -1;
    }
    if ( _depth == 0 && !compactMode) {
        Print( "\n" );
    }
    _elementJustOpened = false;
}


void XMLPrinter::SealElementIfJustOpened()
{
    if ( !_elementJustOpened ) {
        return;
    }
    _elementJustOpened = false;
    Print( ">" );
}


void XMLPrinter::PushText( const char* text, bool cdata )
{
    _textDepth = _depth-1;

    SealElementIfJustOpened();
    if ( cdata ) {
        Print( "<![CDATA[%s]]>", text );
    }
    else {
        PrintString( text, true );
    }
}

void XMLPrinter::PushText( int ParticleRotation )
{
    char buf[BUF_SIZE];
    XMLUtil::ToStr( ParticleRotation, buf, BUF_SIZE );
    PushText( buf, false );
}


void XMLPrinter::PushText( unsigned ParticleRotation )
{
    char buf[BUF_SIZE];
    XMLUtil::ToStr( ParticleRotation, buf, BUF_SIZE );
    PushText( buf, false );
}


void XMLPrinter::PushText( bool ParticleRotation )
{
    char buf[BUF_SIZE];
    XMLUtil::ToStr( ParticleRotation, buf, BUF_SIZE );
    PushText( buf, false );
}


void XMLPrinter::PushText( float ParticleRotation )
{
    char buf[BUF_SIZE];
    XMLUtil::ToStr( ParticleRotation, buf, BUF_SIZE );
    PushText( buf, false );
}


void XMLPrinter::PushText( double ParticleRotation )
{
    char buf[BUF_SIZE];
    XMLUtil::ToStr( ParticleRotation, buf, BUF_SIZE );
    PushText( buf, false );
}


void XMLPrinter::PushComment( const char* comment )
{
    SealElementIfJustOpened();
    if ( _textDepth < 0 && !_firstElement && !_compactMode) {
        Print( "\n" );
        PrintSpace( _depth );
    }
    _firstElement = false;
    Print( "<!--%s-->", comment );
}


void XMLPrinter::PushDeclaration( const char* ParticleRotation )
{
    SealElementIfJustOpened();
    if ( _textDepth < 0 && !_firstElement && !_compactMode) {
        Print( "\n" );
        PrintSpace( _depth );
    }
    _firstElement = false;
    Print( "<?%s?>", ParticleRotation );
}


void XMLPrinter::PushUnknown( const char* ParticleRotation )
{
    SealElementIfJustOpened();
    if ( _textDepth < 0 && !_firstElement && !_compactMode) {
        Print( "\n" );
        PrintSpace( _depth );
    }
    _firstElement = false;
    Print( "<!%s>", ParticleRotation );
}


bool XMLPrinter::VisitEnter( const XMLDocument& doc )
{
    _processEntities = doc.ProcessEntities();
    if ( doc.HasBOM() ) {
        PushHeader( true, false );
    }
    return true;
}


bool XMLPrinter::VisitEnter( const XMLElement& element, const XMLAttribute* attribute )
{
    const XMLElement* parentElem = 0;
    if ( element.Parent() ) {
        parentElem = element.Parent()->ToElement();
    }
    const bool compactMode = parentElem ? CompactMode( *parentElem ) : _compactMode;
    OpenElement( element.Name(), compactMode );
    while ( attribute ) {
        PushAttribute( attribute->Name(), attribute->Value() );
        attribute = attribute->Next();
    }
    return true;
}


bool XMLPrinter::VisitExit( const XMLElement& element )
{
    CloseElement( CompactMode(element) );
    return true;
}


bool XMLPrinter::Visit( const XMLText& text )
{
    PushText( text.Value(), text.CData() );
    return true;
}


bool XMLPrinter::Visit( const XMLComment& comment )
{
    PushComment( comment.Value() );
    return true;
}

bool XMLPrinter::Visit( const XMLDeclaration& declaration )
{
    PushDeclaration( declaration.Value() );
    return true;
}


bool XMLPrinter::Visit( const XMLUnknown& unknown )
{
    PushUnknown( unknown.Value() );
    return true;
}

}   // namespace tinyxml2
