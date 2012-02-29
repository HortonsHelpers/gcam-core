#ifndef _RENEWABLE_SUBRESOURCE_H_
#define _RENEWABLE_SUBRESOURCE_H_
#if defined(_MSC_VER)
#pragma once
#endif

/*
 * LEGAL NOTICE
 * This computer software was prepared by Battelle Memorial Institute,
 * hereinafter the Contractor, under Contract No. DE-AC05-76RL0 1830
 * with the Department of Energy (DOE). NEITHER THE GOVERNMENT NOR THE
 * CONTRACTOR MAKES ANY WARRANTY, EXPRESS OR IMPLIED, OR ASSUMES ANY
 * LIABILITY FOR THE USE OF THIS SOFTWARE. This notice including this
 * sentence must appear on any copies of this computer software.
 * 
 * EXPORT CONTROL
 * User agrees that the Software will not be shipped, transferred or
 * exported into any country or used in any manner prohibited by the
 * United States Export Administration Act or any other applicable
 * export laws, restrictions or regulations (collectively the "Export Laws").
 * Export of the Software may require some form of license or other
 * authority from the U.S. Government, and failure to obtain such
 * export control license may result in criminal liability under
 * U.S. laws. In addition, if the Software is identified as export controlled
 * items under the Export Laws, User represents and warrants that User
 * is not a citizen, or otherwise located within, an embargoed nation
 * (including without limitation Iran, Syria, Sudan, Cuba, and North Korea)
 *     and that User is not otherwise prohibited
 * under the Export Laws from receiving the Software.
 * 
 * All rights to use the Software are granted on condition that such
 * rights are forfeited if User fails to comply with the terms of
 * this Agreement.
 * 
 * User agrees to identify, defend and hold harmless BATTELLE,
 * its officers, agents and employees from all liability involving
 * the violation of such Export Laws, either directly or indirectly,
 * by User.
 */


/*! 
* \file renewable_subresource.h
* \ingroup Objects
* \brief The SubRenewableResource class header file.
* \author Sonny Kim
*/
#include <xercesc/dom/DOMNode.hpp>
#include "resources/include/subresource.h"

// Forward declarations.
class Grade;
class SubResource;
class Tabs;
class IInfo;

/*! 
* \ingroup Objects
* \brief A class which defines a SubRenewableResource object, which is a container for multiple grade objects.
* \author Steve Smith
* \date $ Date $
* \version $ Revision $
*/
class SubRenewableResource: public SubResource {

protected:
    double maxSubResource;
    double gdpSupplyElasticity;
    //! subresource variance now read in rather than computed
    double subResourceVariance;
    //! read in average capacity factor for each subresource
    double subResourceCapacityFactor;  
    virtual const std::string& getXMLName() const;
    virtual bool XMLDerivedClassParse( const std::string& nodeName, const xercesc::DOMNode* node );
    virtual void toXMLforDerivedClass( std::ostream& out, Tabs* tabs ) const;
public: 
    SubRenewableResource();
    virtual void completeInit( const IInfo* aSectorInfo );
    virtual void cumulsupply(double prc,int per);
    virtual void annualsupply( int per, const GDP* gdp, double price1, double price2 );
    virtual double getVariance() const;
    virtual double getMaxSubResource() const;
    virtual double getAverageCapacityFactor() const;
    //! Return the XML tag name
    static const std::string& getXMLNameStatic( void );
    virtual void accept( IVisitor* aVisitor, const int aPeriod ) const;
};
#endif // _RENEWABLE_SUBRESOURCE_H_
