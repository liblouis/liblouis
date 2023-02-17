<?xml version="1.0" encoding="UTF-8"?>
<!--
  Copyright (c) 2022. American Printing House for the Blind, 1839 Frankfort Avenue, Louisville, KY 40206, www.aph.org

  This program is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
  This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.
  You should have received a copy of the GNU Lesser General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.

  -->

<xsl:transform version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
               xmlns:m="http://www.w3.org/1998/Math/MathML">

    <xsl:variable name="math-begin">&#xf32e;</xsl:variable>
    <xsl:variable name="math-end">&#xf32f;</xsl:variable>

    <xsl:variable name="expression-begin">&#xf330;</xsl:variable>
    <xsl:variable name="expression-separator">&#xf331;</xsl:variable>
    <xsl:variable name="expression-end">&#xf332;</xsl:variable>

    <xsl:variable name="minus-sign">&#x2212;</xsl:variable>


    <xsl:variable name="fraction-begin">&#xf500;</xsl:variable>
    <xsl:variable name="fraction-separator">&#xf501;</xsl:variable>
    <xsl:variable name="fraction-separator-bevelled">&#xf502;</xsl:variable>
    <xsl:variable name="fraction-end">&#xf503;</xsl:variable>

    <xsl:variable name="fraction-mixed-begin">&#xf504;</xsl:variable>
    <xsl:variable name="fraction-mixed-separator">&#xf505;</xsl:variable>
    <xsl:variable name="fraction-mixed-separator-bevelled">&#xf506;</xsl:variable>
    <xsl:variable name="fraction-mixed-end">&#xf507;</xsl:variable>

    <xsl:variable name="fraction-complex-begin">&#xf508;</xsl:variable>
    <xsl:variable name="fraction-complex-separator">&#xf509;</xsl:variable>
    <xsl:variable name="fraction-complex-separator-bevelled">&#xf50a;</xsl:variable>
    <xsl:variable name="fraction-complex-end">&#xf50b;</xsl:variable>

    <xsl:variable name="fraction-simple-begin">&#xf50c;</xsl:variable>
    <xsl:variable name="fraction-simple-separator">&#xf50d;</xsl:variable>
    <xsl:variable name="fraction-simple-separator-bevelled">&#xf50e;</xsl:variable>
    <xsl:variable name="fraction-simple-end">&#xf50f;</xsl:variable>


    <xsl:variable name="subscript-begin">&#xf580;</xsl:variable>
    <xsl:variable name="subscript-separator">&#xf581;</xsl:variable>
    <xsl:variable name="subscript-end">&#xf582;</xsl:variable>

    <xsl:variable name="superscript-begin">&#xf583;</xsl:variable>
    <xsl:variable name="superscript-separator">&#xf584;</xsl:variable>
    <xsl:variable name="superscript-end">&#xf585;</xsl:variable>

    <xsl:variable name="subsuperscript-begin">&#xf586;</xsl:variable>
    <xsl:variable name="subsuperscript-separator">&#xf587;</xsl:variable>
    <xsl:variable name="subsuperscript-end">&#xf588;</xsl:variable>

    <xsl:variable name="subsuper-comma">&#xf589;</xsl:variable>


    <xsl:variable name="underscript-begin">&#xf5a0;</xsl:variable>
    <xsl:variable name="underscript-separator">&#xf5a1;</xsl:variable>
    <xsl:variable name="underscript-end">&#xf5a2;</xsl:variable>

    <xsl:variable name="overscript-begin">&#xf5a3;</xsl:variable>
    <xsl:variable name="overscript-separator">&#xf5a4;</xsl:variable>
    <xsl:variable name="overscript-end">&#xf5a5;</xsl:variable>

    <xsl:variable name="underoverscript-begin">&#xf5a6;</xsl:variable>
    <xsl:variable name="underoverscript-separator">&#xf5a7;</xsl:variable>
    <xsl:variable name="underoverscript-end">&#xf5a8;</xsl:variable>


    <xsl:variable name="root-begin">&#xf5b0;</xsl:variable>
    <xsl:variable name="root-separator">&#xf5b1;</xsl:variable>
    <xsl:variable name="root-end">&#xf5b2;</xsl:variable>

    <xsl:variable name="sqrt-begin">&#xf5b3;</xsl:variable>
    <xsl:variable name="sqrt-end">&#xf5b4;</xsl:variable>


    <xsl:variable name="table-begin">&#xf600;</xsl:variable>
    <xsl:variable name="table-end">&#xf601;</xsl:variable>
    <xsl:variable name="table-row-begin">&#xf602;</xsl:variable>
    <xsl:variable name="table-row-end">&#xf603;</xsl:variable>
    <xsl:variable name="table-cell-begin">&#xf604;</xsl:variable>
    <xsl:variable name="table-cell-end">&#xf605;</xsl:variable>

    <xsl:variable name="table-nemeth-round-open">&#xf610;</xsl:variable>
    <xsl:variable name="table-nemeth-round-close">&#xf611;</xsl:variable>
    <xsl:variable name="table-nemeth-square-open">&#xf612;</xsl:variable>
    <xsl:variable name="table-nemeth-square-close">&#xf613;</xsl:variable>
    <xsl:variable name="table-nemeth-curly-open">&#xf614;</xsl:variable>
    <xsl:variable name="table-nemeth-curly-close">&#xf615;</xsl:variable>
    <xsl:variable name="table-nemeth-angled-open">&#xf616;</xsl:variable>
    <xsl:variable name="table-nemeth-angled-close">&#xf617;</xsl:variable>
    <xsl:variable name="table-nemeth-bar">&#xf618;</xsl:variable>


    <xsl:template match="node()|@*">
        <xsl:copy>
            <xsl:apply-templates select="node()|@*"/>
        </xsl:copy>
    </xsl:template>
    <xsl:template match="m:math">
        <xsl:value-of select="$math-begin"/>
        <xsl:apply-templates select="*">
            <xsl:with-param name="insubsup" select="0"/>
        </xsl:apply-templates>
        <xsl:value-of select="$math-end"/>
    </xsl:template>

    <xsl:template match="m:mn|m:mi">
        <xsl:value-of select="."/>
    </xsl:template>
    <xsl:template match="m:mo">
        <!--<xsl:value-of select="."/>-->
        <xsl:choose>
            <!--<xsl:when test=". = '-'">-->
            <xsl:when test="contains(., '-')">
                <xsl:text>&#x2212;</xsl:text>
            </xsl:when>
            <xsl:otherwise>
                <xsl:value-of select="."/>
            </xsl:otherwise>
        </xsl:choose>
        <!--<xsl:param name="insubsup" />
        <xsl:if test="not(preceding-sibling::m:mtable or following-sibling::m:mtable)">
            <xsl:choose>
                <xsl:when test=". = ',' and $insubsup > 0">
                    <xsl:value-of select="$subsuper-comma"/>
                </xsl:when>
                <xsl:when test=". = '-'">
                    <xsl:value-of select="$minus-sign"/>
                </xsl:when>
                <xsl:otherwise>
                    <xsl:value-of select="."/>
                </xsl:otherwise>
            </xsl:choose>
        </xsl:if>-->
    </xsl:template>

    <xsl:template match="m:mrow">
        <xsl:param name="insubsup"/>
        <xsl:param name="operator-begin"/>
        <xsl:param name="operator-end"/>
        <xsl:value-of select="$expression-begin"/>
        <xsl:apply-templates select="*">
            <xsl:with-param name="insubsup" select="$insubsup"/>
            <xsl:with-param name="operator-begin" select="$operator-begin"/>
            <xsl:with-param name="operator-end" select="$operator-end"/>
        </xsl:apply-templates>
        <xsl:value-of select="$expression-end"/>
    </xsl:template>

    <xsl:template match="m:mfrac">
        <xsl:param name="insubsup"/>
        <xsl:param name="operator-begin"/>
        <xsl:param name="operator-end"/>
        <xsl:choose>
            <xsl:when test="local-name(*[1]) = 'mfrac' or local-name(*[2]) = 'mfrac'">
                <xsl:value-of select="$fraction-complex-begin"/>
                <xsl:apply-templates select="*[1]">
                    <xsl:with-param name="insubsup" select="$insubsup"/>
                    <xsl:with-param name="operator-begin" select="$operator-begin"/>
                    <xsl:with-param name="operator-end" select="$operator-end"/>
                </xsl:apply-templates>
                <xsl:value-of select="$fraction-complex-separator"/>
                <xsl:apply-templates select="*[2]">
                    <xsl:with-param name="insubsup" select="$insubsup"/>
                    <xsl:with-param name="operator-begin" select="$operator-begin"/>
                    <xsl:with-param name="operator-end" select="$operator-end"/>
                </xsl:apply-templates>
                <xsl:value-of select="$fraction-complex-end"/>
            </xsl:when>
            <xsl:when
                    test="(local-name(preceding-sibling::*[1]) = 'mn' or preceding-sibling::*[1] = '&#x2064;') and local-name(*[1]) = 'mn' and local-name(*[2]) = 'mn' and not(local-name(..) = 'mfrac')">
                <xsl:value-of select="$fraction-mixed-begin"/>
                <xsl:apply-templates select="*[1]">
                    <xsl:with-param name="insubsup" select="$insubsup"/>
                    <xsl:with-param name="operator-begin" select="$operator-begin"/>
                    <xsl:with-param name="operator-end" select="$operator-end"/>
                </xsl:apply-templates>
                <xsl:value-of select="$fraction-mixed-separator"/>
                <xsl:apply-templates select="*[2]">
                    <xsl:with-param name="insubsup" select="$insubsup"/>
                    <xsl:with-param name="operator-begin" select="$operator-begin"/>
                    <xsl:with-param name="operator-end" select="$operator-end"/>
                </xsl:apply-templates>
                <xsl:value-of select="$fraction-mixed-end"/>
            </xsl:when>
            <xsl:when test="local-name(*[1]) = 'mn' and local-name(*[2]) = 'mn'">
                <xsl:value-of select="$fraction-simple-begin"/>
                <xsl:apply-templates select="*[1]">
                    <xsl:with-param name="insubsup" select="$insubsup"/>
                    <xsl:with-param name="operator-begin" select="$operator-begin"/>
                    <xsl:with-param name="operator-end" select="$operator-end"/>
                </xsl:apply-templates>
                <xsl:value-of select="$fraction-simple-separator"/>
                <xsl:apply-templates select="*[2]">
                    <xsl:with-param name="insubsup" select="$insubsup"/>
                    <xsl:with-param name="operator-begin" select="$operator-begin"/>
                    <xsl:with-param name="operator-end" select="$operator-end"/>
                </xsl:apply-templates>
                <xsl:value-of select="$fraction-simple-end"/>
            </xsl:when>
            <xsl:otherwise>
                <xsl:value-of select="$fraction-begin"/>
                <xsl:apply-templates select="*[1]">
                    <xsl:with-param name="insubsup" select="$insubsup"/>
                    <xsl:with-param name="operator-begin" select="$operator-begin"/>
                    <xsl:with-param name="operator-end" select="$operator-end"/>
                </xsl:apply-templates>
                <xsl:value-of select="$fraction-separator"/>
                <xsl:apply-templates select="*[2]">
                    <xsl:with-param name="insubsup" select="$insubsup"/>
                    <xsl:with-param name="operator-begin" select="$operator-begin"/>
                    <xsl:with-param name="operator-end" select="$operator-end"/>
                </xsl:apply-templates>
                <xsl:value-of select="$fraction-end"/>
            </xsl:otherwise>
        </xsl:choose>
    </xsl:template>
    <xsl:template match="m:mfrac[@bevelled='true']">
        <xsl:param name="insubsup"/>
        <xsl:param name="operator-begin"/>
        <xsl:param name="operator-end"/>
        <xsl:choose>
            <xsl:when test="local-name(*[1]) = 'mfrac' or local-name(*[2]) = 'mfrac'">
                <xsl:value-of select="$fraction-complex-begin"/>
                <xsl:apply-templates select="*[1]">
                    <xsl:with-param name="insubsup" select="$insubsup"/>
                    <xsl:with-param name="operator-begin" select="$operator-begin"/>
                    <xsl:with-param name="operator-end" select="$operator-end"/>
                </xsl:apply-templates>
                <xsl:value-of select="$fraction-complex-separator-bevelled"/>
                <xsl:apply-templates select="*[2]">
                    <xsl:with-param name="insubsup" select="$insubsup"/>
                    <xsl:with-param name="operator-begin" select="$operator-begin"/>
                    <xsl:with-param name="operator-end" select="$operator-end"/>
                </xsl:apply-templates>
                <xsl:value-of select="$fraction-complex-end"/>
            </xsl:when>
            <xsl:when
                    test="(local-name(preceding-sibling::*[1]) or preceding-sibling::*[1] = '&#x2064;') and local-name(*[1]) = 'mn' and local-name(*[2]) = 'mn' and not(local-name(..) = 'mfrac')">
                <xsl:value-of select="$fraction-mixed-begin"/>
                <xsl:apply-templates select="*[1]">
                    <xsl:with-param name="insubsup" select="$insubsup"/>
                    <xsl:with-param name="operator-begin" select="$operator-begin"/>
                    <xsl:with-param name="operator-end" select="$operator-end"/>
                </xsl:apply-templates>
                <xsl:value-of select="$fraction-mixed-separator-bevelled"/>
                <xsl:apply-templates select="*[2]">
                    <xsl:with-param name="insubsup" select="$insubsup"/>
                    <xsl:with-param name="operator-begin" select="$operator-begin"/>
                    <xsl:with-param name="operator-end" select="$operator-end"/>
                </xsl:apply-templates>
                <xsl:value-of select="$fraction-mixed-end"/>
            </xsl:when>
            <xsl:when test="local-name(*[1]) = 'mn' and local-name(*[2]) = 'mn'">
                <xsl:value-of select="$fraction-simple-begin"/>
                <xsl:apply-templates select="*[1]">
                    <xsl:with-param name="insubsup" select="$insubsup"/>
                    <xsl:with-param name="operator-begin" select="$operator-begin"/>
                    <xsl:with-param name="operator-end" select="$operator-end"/>
                </xsl:apply-templates>
                <xsl:value-of select="$fraction-simple-separator-bevelled"/>
                <xsl:apply-templates select="*[2]">
                    <xsl:with-param name="insubsup" select="$insubsup"/>
                    <xsl:with-param name="operator-begin" select="$operator-begin"/>
                    <xsl:with-param name="operator-end" select="$operator-end"/>
                </xsl:apply-templates>
                <xsl:value-of select="$fraction-simple-end"/>
            </xsl:when>
            <xsl:otherwise>
                <xsl:value-of select="$fraction-begin"/>
                <xsl:apply-templates select="*[1]">
                    <xsl:with-param name="insubsup" select="$insubsup"/>
                    <xsl:with-param name="operator-begin" select="$operator-begin"/>
                    <xsl:with-param name="operator-end" select="$operator-end"/>
                </xsl:apply-templates>
                <xsl:value-of select="$fraction-separator-bevelled"/>
                <xsl:apply-templates select="*[2]">
                    <xsl:with-param name="insubsup" select="$insubsup"/>
                    <xsl:with-param name="operator-begin" select="$operator-begin"/>
                    <xsl:with-param name="operator-end" select="$operator-end"/>
                </xsl:apply-templates>
                <xsl:value-of select="$fraction-end"/>
            </xsl:otherwise>
        </xsl:choose>
    </xsl:template>

    <xsl:template match="m:msub">
        <xsl:param name="insubsup"/>
        <xsl:param name="operator-begin"/>
        <xsl:param name="operator-end"/>
        <xsl:value-of select="$subscript-begin"/>
        <xsl:apply-templates select="node()[1]">
            <xsl:with-param name="insubsup" select="$insubsup"/>
            <xsl:with-param name="operator-begin" select="$operator-begin"/>
            <xsl:with-param name="operator-end" select="$operator-end"/>
        </xsl:apply-templates>
        <xsl:call-template name="subsup_loop">
            <xsl:with-param name="separator" select="$subscript-separator"/>
            <xsl:with-param name="count" select="$insubsup + 1"/>
        </xsl:call-template>
        <xsl:apply-templates select="node()[2]">
            <xsl:with-param name="insubsup" select="$insubsup + 1"/>
            <xsl:with-param name="operator-begin" select="$operator-begin"/>
            <xsl:with-param name="operator-end" select="$operator-end"/>
        </xsl:apply-templates>
        <xsl:value-of select="$subscript-end"/>
    </xsl:template>
    <xsl:template match="m:msup">
        <xsl:param name="insubsup"/>
        <xsl:param name="operator-begin"/>
        <xsl:param name="operator-end"/>
        <xsl:value-of select="$superscript-begin"/>
        <xsl:apply-templates select="node()[1]">
            <xsl:with-param name="insubsup" select="$insubsup + 1"/>
            <xsl:with-param name="operator-begin" select="$operator-begin"/>
            <xsl:with-param name="operator-end" select="$operator-end"/>
        </xsl:apply-templates>
        <xsl:call-template name="subsup_loop">
            <xsl:with-param name="separator" select="$superscript-separator"/>
            <xsl:with-param name="count" select="$insubsup + 1"/>
        </xsl:call-template>
        <xsl:apply-templates select="node()[2]">
            <xsl:with-param name="insubsup" select="$insubsup + 1"/>
            <xsl:with-param name="operator-begin" select="$operator-begin"/>
            <xsl:with-param name="operator-end" select="$operator-end"/>
        </xsl:apply-templates>
        <xsl:value-of select="$superscript-end"/>
    </xsl:template>
    <xsl:template match="m:msubsup">
        <xsl:param name="insubsup"/>
        <xsl:param name="operator-begin"/>
        <xsl:param name="operator-end"/>
        <xsl:value-of select="$subsuperscript-begin"/>
        <xsl:apply-templates select="node()[1]">
            <xsl:with-param name="insubsup" select="$insubsup"/>
            <xsl:with-param name="operator-begin" select="$operator-begin"/>
            <xsl:with-param name="operator-end" select="$operator-end"/>
        </xsl:apply-templates>
        <xsl:value-of select="$subsuperscript-separator"/>
        <xsl:apply-templates select="node()[2]">
            <xsl:with-param name="insubsup" select="$insubsup + 1"/>
            <xsl:with-param name="operator-begin" select="$operator-begin"/>
            <xsl:with-param name="operator-end" select="$operator-end"/>
        </xsl:apply-templates>
        <xsl:value-of select="$subsuperscript-separator"/>
        <xsl:apply-templates select="node()[3]">
            <xsl:with-param name="insubsup" select="$insubsup + 1"/>
            <xsl:with-param name="operator-begin" select="$operator-begin"/>
            <xsl:with-param name="operator-end" select="$operator-end"/>
        </xsl:apply-templates>
        <xsl:value-of select="$subsuperscript-end"/>
    </xsl:template>
    <xsl:template name="subsup_loop">
        <xsl:param name="separator"/>
        <xsl:param name="count"/>
        <xsl:if test="$count > 0">
            <xsl:value-of select="$separator"/>
            <xsl:call-template name="subsup_loop">
                <xsl:with-param name="separator" select="$separator"/>
                <xsl:with-param name="count" select="$count - 1"/>
            </xsl:call-template>
        </xsl:if>
    </xsl:template>

    <xsl:template match="m:munder">
        <xsl:param name="insubsup"/>
        <xsl:param name="operator-begin"/>
        <xsl:param name="operator-end"/>
        <xsl:value-of select="$underscript-begin"/>
        <xsl:apply-templates select="*[1]">
            <xsl:with-param name="insubsup" select="$insubsup"/>
            <xsl:with-param name="operator-begin" select="$operator-begin"/>
            <xsl:with-param name="operator-end" select="$operator-end"/>
        </xsl:apply-templates>
        <xsl:value-of select="$underscript-separator"/>
        <xsl:apply-templates select="*[2]">
            <xsl:with-param name="insubsup" select="$insubsup"/>
            <xsl:with-param name="operator-begin" select="$operator-begin"/>
            <xsl:with-param name="operator-end" select="$operator-end"/>
        </xsl:apply-templates>
        <xsl:value-of select="$underscript-end"/>
    </xsl:template>
    <xsl:template match="m:mover">
        <xsl:param name="insubsup"/>
        <xsl:param name="operator-begin"/>
        <xsl:param name="operator-end"/>
        <xsl:value-of select="$overscript-begin"/>
        <xsl:apply-templates select="*[1]">
            <xsl:with-param name="insubsup" select="$insubsup"/>
            <xsl:with-param name="operator-begin" select="$operator-begin"/>
            <xsl:with-param name="operator-end" select="$operator-end"/>
        </xsl:apply-templates>
        <xsl:value-of select="$overscript-separator"/>
        <xsl:apply-templates select="*[2]">
            <xsl:with-param name="insubsup" select="$insubsup"/>
            <xsl:with-param name="operator-begin" select="$operator-begin"/>
            <xsl:with-param name="operator-end" select="$operator-end"/>
        </xsl:apply-templates>
        <xsl:value-of select="$overscript-end"/>
    </xsl:template>
    <xsl:template match="m:munderover">
        <xsl:param name="insubsup"/>
        <xsl:param name="operator-begin"/>
        <xsl:param name="operator-end"/>
        <xsl:value-of select="$underoverscript-begin"/>
        <xsl:apply-templates select="*[1]">
            <xsl:with-param name="insubsup" select="$insubsup"/>
            <xsl:with-param name="operator-begin" select="$operator-begin"/>
            <xsl:with-param name="operator-end" select="$operator-end"/>
        </xsl:apply-templates>
        <xsl:value-of select="$underscript-separator"/>
        <xsl:apply-templates select="*[2]">
            <xsl:with-param name="insubsup" select="$insubsup"/>
            <xsl:with-param name="operator-begin" select="$operator-begin"/>
            <xsl:with-param name="operator-end" select="$operator-end"/>
        </xsl:apply-templates>
        <xsl:value-of select="$overscript-separator"/>
        <xsl:apply-templates select="*[3]">
            <xsl:with-param name="insubsup" select="$insubsup"/>
            <xsl:with-param name="operator-begin" select="$operator-begin"/>
            <xsl:with-param name="operator-end" select="$operator-end"/>
        </xsl:apply-templates>
        <xsl:value-of select="$underoverscript-end"/>
    </xsl:template>

    <xsl:template match="m:msqrt">
        <xsl:param name="insubsup"/>
        <xsl:param name="operator-begin"/>
        <xsl:param name="operator-end"/>
        <xsl:value-of select="$sqrt-begin"/>
        <xsl:apply-templates select="*">
            <xsl:with-param name="insubsup" select="$insubsup"/>
            <xsl:with-param name="operator-begin" select="$operator-begin"/>
            <xsl:with-param name="operator-end" select="$operator-end"/>
        </xsl:apply-templates>
        <xsl:value-of select="$sqrt-end"/>
    </xsl:template>
    <xsl:template match="m:mroot">
        <xsl:param name="insubsup"/>
        <xsl:param name="operator-begin"/>
        <xsl:param name="operator-end"/>
        <xsl:value-of select="$root-begin"/>
        <xsl:apply-templates select="*[2]">
            <xsl:with-param name="insubsup" select="$insubsup"/>
            <xsl:with-param name="operator-begin" select="$operator-begin"/>
            <xsl:with-param name="operator-end" select="$operator-end"/>
        </xsl:apply-templates>
        <xsl:value-of select="$root-separator"/>
        <xsl:apply-templates select="*[1]">
            <xsl:with-param name="insubsup" select="$insubsup"/>
            <xsl:with-param name="operator-begin" select="$operator-begin"/>
            <xsl:with-param name="operator-end" select="$operator-end"/>
        </xsl:apply-templates>
        <xsl:value-of select="$root-end"/>
    </xsl:template>

    <xsl:template match="m:mtable">
        <xsl:param name="insubsup"/>
        <xsl:value-of select="$table-begin"/>
        <xsl:apply-templates select="*">
            <xsl:with-param name="insubsup" select="$insubsup"/>
            <xsl:with-param name="operator-begin" select="preceding-sibling::*[1]"/>
            <xsl:with-param name="operator-end" select="following-sibling::*[1]"/>
        </xsl:apply-templates>
        <xsl:value-of select="$table-end"/>
    </xsl:template>
    <xsl:template match="m:mtr">
        <xsl:param name="insubsup"/>
        <xsl:param name="operator-begin"/>
        <xsl:param name="operator-end"/>
        <xsl:value-of select="$table-row-begin"/>
        <!--<xsl:value-of select="$operator-begin"/>-->
        <xsl:choose>
            <xsl:when test="$operator-begin = '('">
                <xsl:value-of select="$table-nemeth-round-open"/>
            </xsl:when>
            <xsl:when test="$operator-begin = '['">
                <xsl:value-of select="$table-nemeth-square-open"/>
            </xsl:when>
            <xsl:when test="$operator-begin = '{'">
                <xsl:value-of select="$table-nemeth-curly-open"/>
            </xsl:when>
            <xsl:when test="$operator-begin = '&lt;'">
                <xsl:value-of select="$table-nemeth-angled-open"/>
            </xsl:when>
            <xsl:when test="$operator-begin = '|'">
                <xsl:value-of select="$table-nemeth-bar"/>
            </xsl:when>
            <xsl:otherwise>
                <xsl:value-of select="$operator-begin"/>
            </xsl:otherwise>
        </xsl:choose>
        <xsl:apply-templates select="*">
            <xsl:with-param name="insubsup" select="$insubsup"/>
            <xsl:with-param name="operator-begin" select="$operator-begin"/>
            <xsl:with-param name="operator-end" select="$operator-end"/>
        </xsl:apply-templates>
        <!--<xsl:value-of select="$operator-end"/>-->
        <xsl:choose>
            <xsl:when test="$operator-end = ')'">
                <xsl:value-of select="$table-nemeth-round-close"/>
            </xsl:when>
            <xsl:when test="$operator-end = ']'">
                <xsl:value-of select="$table-nemeth-square-close"/>
            </xsl:when>
            <xsl:when test="$operator-end = '}'">
                <xsl:value-of select="$table-nemeth-curly-close"/>
            </xsl:when>
            <xsl:when test="$operator-end = '&gt;'">
                <xsl:value-of select="$table-nemeth-angled-close"/>
            </xsl:when>
            <xsl:when test="$operator-end = '|'">
                <xsl:value-of select="$table-nemeth-bar"/>
            </xsl:when>
            <xsl:otherwise>
                <xsl:value-of select="$operator-end"/>
            </xsl:otherwise>
        </xsl:choose>
        <xsl:value-of select="$table-row-end"/>
    </xsl:template>
    <xsl:template match="m:mtd">
        <xsl:param name="insubsup"/>
        <xsl:param name="operator-begin"/>
        <xsl:param name="operator-end"/>
        <xsl:value-of select="$table-cell-begin"/>
        <xsl:apply-templates select="*">
            <xsl:with-param name="insubsup" select="$insubsup"/>
            <xsl:with-param name="operator-begin" select="$operator-begin"/>
            <xsl:with-param name="operator-end" select="$operator-end"/>
        </xsl:apply-templates>
        <xsl:value-of select="$table-cell-end"/>
    </xsl:template>

    <xsl:template match="m:mfenced">
        <xsl:param name="insubsup"/>
        <xsl:param name="operator-begin"/>
        <xsl:param name="operator-end"/>
        <xsl:value-of select="$expression-begin"/>
        <xsl:choose>
            <xsl:when test="@open">
                <xsl:value-of select="@open"/>
            </xsl:when>
            <xsl:otherwise>
                <xsl:text>(</xsl:text>
            </xsl:otherwise>
        </xsl:choose>
        <xsl:for-each select="*">
            <xsl:apply-templates select=".">
                <xsl:with-param name="insubsup" select="$insubsup"/>
                <xsl:with-param name="operator-begin" select="$operator-begin"/>
                <xsl:with-param name="operator-end" select="$operator-end"/>
            </xsl:apply-templates>
            <xsl:if test="position() &lt; last()">
                <xsl:value-of select="$expression-separator"/>
                <xsl:choose>
                    <xsl:when test="../@separators">
                        <xsl:choose>
                            <xsl:when test="string-length(../@separators) &gt; position()">
                                <xsl:value-of select="substring(../@separators, position(), 1)"/>
                            </xsl:when>
                            <xsl:otherwise>
                                <xsl:value-of select="substring(../@separators, string-length(../@separators), 1)"/>
                            </xsl:otherwise>
                        </xsl:choose>
                    </xsl:when>
                    <xsl:otherwise>
                        <xsl:text>,</xsl:text>
                    </xsl:otherwise>
                </xsl:choose>
            </xsl:if>
        </xsl:for-each>
        <xsl:choose>
            <xsl:when test="@close">
                <xsl:value-of select="@close"/>
            </xsl:when>
            <xsl:otherwise>
                <xsl:text>)</xsl:text>
            </xsl:otherwise>
        </xsl:choose>
        <xsl:value-of select="$expression-end"/>
    </xsl:template>

    <xsl:template match="m:menclose">
        <xsl:param name="insubsup"/>
        <xsl:param name="operator-begin"/>
        <xsl:param name="operator-end"/>
        <xsl:if test="contains(@notation, 'radical')">
            <xsl:value-of select="$expression-begin"/>
        </xsl:if>
        <xsl:apply-templates select="*">
            <xsl:with-param name="insubsup" select="$insubsup"/>
            <xsl:with-param name="operator-begin" select="$operator-begin"/>
            <xsl:with-param name="operator-end" select="$operator-end"/>
        </xsl:apply-templates>
        <xsl:if test="contains(@notation, 'radical')">
            <xsl:value-of select="$expression-end"/>
        </xsl:if>
    </xsl:template>

    <xsl:template match="m:merror|m:mphantum|m:annotation"/>

</xsl:transform>
