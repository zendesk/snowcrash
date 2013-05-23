//
//  ListUtility.h
//  snowcrash
//
//  Created by Zdenek Nemec on 5/12/13.
//  Copyright (c) 2013 Apiary.io. All rights reserved.
//

#ifndef SNOWCRASH_LISTUTILITY_H
#define SNOWCRASH_LISTUTILITY_H

#include <vector>
#include <string>
#include <sstream>
#include "MarkdownBlock.h"
#include "BlueprintParserCore.h"

namespace snowcrash {
    
    // Pair of content parts
    typedef std::vector<MarkdownBlock::Content> ContentParts;
        
    // Extract first line from block content
    // ContentParts[0] contains first line of block content
    // ContentParts[1] contains the rest of block content
    inline ContentParts ExtractFirstLine(const MarkdownBlock& block) {
        
        ContentParts result;
        if (block.content.empty())
            return result;
        
        std::string::size_type pos = block.content.find("\n");
        if (pos == std::string::npos) {
            result.push_back(block.content);
            return result;
        }
        
        result.push_back(block.content.substr(0, pos + 1));
        result.push_back(block.content.substr(pos + 1, std::string::npos));
        return result;
    }
    
    // Return first list / list item content block block (paragraph)
    // This is the first block inside the list / list item or the list item's closing block 
    inline BlockIterator FirstContentBlock(const BlockIterator& begin,
                                           const BlockIterator& end) {
        
        BlockIterator cur = begin;
        if (cur->type == ListBlockBeginType)
            if (++cur == end)
                return end;
        
        if (cur->type == ListItemBlockBeginType)
            if (++cur == end)
                return end;
        
        return cur;
    }

    // Return name block of list item; that is either FirstContentBlock() or
    // matching closing items block for inline items
    inline BlockIterator ListItemNameBlock(const BlockIterator& begin,
                                           const BlockIterator& end) {
        
        BlockIterator cur = FirstContentBlock(begin, end);
        if (cur == end ||
            cur->type != ListBlockBeginType)
            return cur;
        
        // Inline list block
        cur = SkipToSectionEnd(cur, end, ListBlockBeginType, ListBlockEndType);
        if (cur != end)
            return ++cur;
        
        return cur;
    }
    
    // Eats closing elements of a list / list item block
    inline BlockIterator CloseListItemBlock(const BlockIterator& begin,
                                            const BlockIterator& end) {
        
        BlockIterator cur = begin;
        if (cur != end &&
            cur->type == ListItemBlockEndType) {
            ++cur; // eat list item end
        }
        
        if (cur != end &&
            cur->type == ListBlockEndType) {
            ++cur; // eat list end
        }
        
        return cur;
    }
    
    // Generic parser handler to warn & skip foreign blocks
    inline ParseSectionResult HandleForeignSection(const BlockIterator& cur,
                                                   const SectionBounds& bounds) {

        ParseSectionResult result = std::make_pair(Result(), cur);
        if (cur->type == ListItemBlockBeginType) {

            result.second = SkipToSectionEnd(cur, bounds.second, ListItemBlockBeginType, ListItemBlockEndType);
            result.first.warnings.push_back(Warning("ignoring unrecognized list item",
                                                    0,
                                                    result.second->sourceMap));
            result.second = CloseListItemBlock(result.second, bounds.second);
        }
        else if (cur->type == ListBlockBeginType) {

            result.second = SkipToSectionEnd(cur, bounds.second, ListBlockBeginType, ListBlockEndType);
            result.first.warnings.push_back(Warning("ignoring unrecognized list",
                                                    0,
                                                    result.second->sourceMap));
            result.second = CloseListItemBlock(result.second, bounds.second);
        }
        else {
            if (cur->type == QuoteBlockBeginType) {
                result.second = SkipToSectionEnd(cur, bounds.second, QuoteBlockBeginType, QuoteBlockEndType);
            }
            else {
                ++result.second;
            }
            result.first.warnings.push_back(Warning("ignoring unrecognized block, check indentation",
                                                    0,
                                                    result.second->sourceMap));
        }

        return result;
    }
    
    // Skips to the end of a list in description, checks
    template <class T>
    static BlockIterator SkipToDescriptionListEnd(const BlockIterator& begin,
                                                  const BlockIterator& end,
                                                  Result& result) {
        BlockIterator cur(begin);
        if (++cur == end)
            return cur;
        
        while (cur != end &&
               cur->type == ListItemBlockBeginType) {
            
            Section listSection = ClassifyInternaListBlock<T>(cur, end);
            cur = SkipToSectionEnd(cur, end, ListItemBlockBeginType, ListItemBlockEndType);
            
            if (listSection != UndefinedSection) {
                // WARN: skipping section in description
                std::stringstream ss;
                ss << "ignoring " << SectionName(listSection);
                ss << " in description, description should not end with list";
                result.warnings.push_back(Warning(ss.str(),
                                                  0,
                                                  (cur != end) ? cur->sourceMap : MakeSourceDataBlock(0,0)));
            }
            if (cur != end)
                ++cur;
        }
        
        return cur;
    }
    
    // Parse preformatted source data from block(s) of a list item block
    // TODO: refactor
    inline ParseSectionResult ParseListPreformattedBlock(const Section& section,
                                                         const BlockIterator& cur,
                                                         const SectionBounds& bounds,
                                                         const SourceData& sourceData,
                                                         SourceData& data,
                                                         SourceDataBlock& sourceMap) {
        
        static const std::string FormattingWarning = "content is expected to be preformatted code block";
        
        ParseSectionResult result = std::make_pair(Result(), cur);
        BlockIterator sectionCur = cur;
        std::stringstream dataStream;
        
        if (sectionCur == bounds.first) {
            // Process first block of list, throw away first line - signature
            sectionCur = ListItemNameBlock(cur, bounds.second);
            if (sectionCur == bounds.second)
                return std::make_pair(Result(), sectionCur);
            
            ContentParts content = ExtractFirstLine(*sectionCur);
            if (content.empty() ||
                content.front().empty()) {
                
                std::stringstream ss;
                ss << "unable to parse " << SectionName(section) << " signature";
                result.first.error = Error(ss.str(),
                                           1,
                                           sectionCur->sourceMap);
                result.second = sectionCur;
                return result;
            }
            
            // Retrieve any extra lines after signature
            if (content.size() == 2 && !content[1].empty()) {
                dataStream << content[1];
                
                // WARN: Not a preformatted code block
                std::stringstream ss;
                ss << SectionName(section) << " " << FormattingWarning;
                result.first.warnings.push_back(Warning(ss.str(),
                                                        0,
                                                        sectionCur->sourceMap));
            }
            
            sectionCur = FirstContentBlock(cur, bounds.second);
        }
        else if (sectionCur->type == CodeBlockType) {

            dataStream << sectionCur->content; // well formatted content, stream it up
        }
        else {
            // Other blocks, process them but warn
            if (sectionCur->type == QuoteBlockBeginType) {
                sectionCur = SkipToSectionEnd(sectionCur, bounds.second, QuoteBlockBeginType, QuoteBlockEndType);
            }
            else if (sectionCur->type == ListBlockBeginType) {
                sectionCur = SkipToSectionEnd(sectionCur, bounds.second, ListBlockBeginType, ListBlockEndType);
            }
            
            dataStream << MapSourceData(sourceData, sectionCur->sourceMap);
            
            // WARN: Not a preformatted code block
            std::stringstream ss;
            ss << SectionName(section) << " " << FormattingWarning;
            result.first.warnings.push_back(Warning(ss.str(),
                                                    0,
                                                    sectionCur->sourceMap));
        }
        
        data = dataStream.str();
        sourceMap = sectionCur->sourceMap;
        
        if (sectionCur != bounds.second)
            result.second = ++sectionCur;
        
        return result;
    }
}

#endif