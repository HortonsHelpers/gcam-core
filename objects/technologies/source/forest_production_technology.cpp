/*!
* \file forest_production_technology.cpp
* \ingroup Objects
* \brief ForestProductionTechnology class source file.
* \author James Blackwood
*/

#include "util/base/include/definitions.h"
#include "technologies/include/forest_production_technology.h"
#include "land_allocator/include/iland_allocator.h"
#include "emissions/include/aghg.h"
#include "containers/include/scenario.h"
#include "containers/include/iinfo.h"
#include "util/base/include/xml_helper.h"
#include "marketplace/include/marketplace.h"
#include "technologies/include/ical_data.h"
#include "technologies/include/iproduction_state.h"
#include "technologies/include/ioutput.h"

using namespace std;
using namespace xercesc;

extern Scenario* scenario;

/*! 
 * \brief Constructor.
 * \param aName Technology name.
 * \param aYear Technology year.
 */
ForestProductionTechnology::ForestProductionTechnology( const string& aName, const int aYear )
:FoodProductionTechnology( aName, aYear ){
    // TODO: 0.02 should not be a default value.
    interestRate = 0.02;
    mRotationPeriod = 0;
}

// ! Destructor
ForestProductionTechnology::~ForestProductionTechnology() {
}

//! Parses any input variables specific to derived classes
bool ForestProductionTechnology::XMLDerivedClassParse( const string& nodeName, const DOMNode* curr ) {
    if( nodeName == "interestRate" ) {
        interestRate = XMLHelper<int>::getValue( curr );
    }
    else if( nodeName == "futureProduction" ) {
        mFutureProduction = XMLHelper<double>::getValue( curr );
    }
    else if( !FoodProductionTechnology::XMLDerivedClassParse(nodeName, curr)) {
        return false;
    }
    return true;
}

//! write object to xml output stream
void ForestProductionTechnology::toInputXMLDerived( ostream& out, Tabs* tabs ) const {
    FoodProductionTechnology::toInputXMLDerived( out, tabs);
    if( mFutureProduction.isInited() ){
        XMLWriteElement( mFutureProduction, "futureProduction", out, tabs );
    }
    XMLWriteElementCheckDefault( interestRate, "interestRate", out, tabs, 0.02 );
}

//! write object to xml output stream
void ForestProductionTechnology::toDebugXMLDerived( const int period, ostream& out, Tabs* tabs ) const {
    FoodProductionTechnology::toDebugXMLDerived( period, out, tabs);
    XMLWriteElement( mFutureProduction, "futureProduction", out, tabs );
    XMLWriteElement( interestRate, "interestRate", out, tabs );
}

/*! \brief Get the XML node name for output to XML.
*
* This public function accesses the private constant string, XML_NAME.
* This way the tag is always consistent for both read-in and output and can be easily changed.
* This function may be virtual to be overridden by derived class pointers.
* \author Josh Lurz, James Blackwood
* \return The constant XML_NAME.
*/
const string& ForestProductionTechnology::getXMLName1D() const {
    return getXMLNameStatic1D();
}

/*! \brief Get the XML node name in static form for comparison when parsing XML.
*
* This public function accesses the private constant string, XML_NAME.
* This way the tag is always consistent for both read-in and output and can be easily changed.
* The "==" operator that is used when parsing, required this second function to return static.
* \note A function cannot be static and virtual.
* \author Josh Lurz, James Blackwood
* \return The constant XML_NAME as a static.
*/
const string& ForestProductionTechnology::getXMLNameStatic1D() {
    const static string XML_NAME = "ForestProductionTechnology";
    return XML_NAME;
}

//! Clone Function. Returns a deep copy of the current technology.
ForestProductionTechnology* ForestProductionTechnology::clone() const {
    return new ForestProductionTechnology( *this );
}

/*! 
* \brief Perform initializations that only need to be done once per period.
* \param aRegionName Region name.
* \param aSectorName Sector name, also the name of the product.
* \param aSubsectorInfo Parent information container.
* \param aDemographics Regional demographics container.
* \param aPeriod Model period.
*/
void ForestProductionTechnology::initCalc( const string& aRegionName,
                                           const string& aSectorName,
                                           const IInfo* aSubsectorInfo,
                                           const Demographic* aDemographics,
                                           const int aPeriod )
{
    // Ideally this would use the production state but it isn't setup yet for
    // this period.
    if( year == scenario->getModeltime()->getper_to_yr( aPeriod ) ){
        // Set calibrated values to land allocator in case these were disrupted
        // in previous period
        setCalLandValues();
    }

    FoodProductionTechnology::initCalc( aRegionName, aSectorName, aSubsectorInfo,
                                        aDemographics, aPeriod );
}

/*!
* \brief Complete the initialization of the technology.
* \note This routine is only called once per model run
* \param aRegionName Region name.
* \param aSectorName Sector name, also the name of the product.
* \param aDepDefinder Regional dependency finder.
* \param aSubsectorInfo Subsector information object.
* \param aLandAllocator Regional land allocator.
* \param aGlobalTechDB Global Technology database.
* \author Josh Lurz
* \warning Markets are not necessarily set when completeInit is called
* \author James Blackwood
* \warning This may break if timestep is not constant for each time period.
*/
void ForestProductionTechnology::completeInit( const string& aRegionName,
                                               const string& aSectorName,
                                               DependencyFinder* aDepFinder,
                                               const IInfo* aSubsectorInfo,
                                               ILandAllocator* aLandAllocator,
                                               const GlobalTechnologyDatabase* aGlobalTechDB )
{
    // Setup the land allocators for the secondary outputs
    if ( mOutputs.size() ) {
        // Technology::completeInit() will add the primary output.
        // At this point, all are secondary outputs
        for ( vector<IOutput*>::iterator outputIter = mOutputs.begin(); outputIter != mOutputs.end(); ++outputIter ) {
           ( *outputIter )->setLandAllocator( aLandAllocator, mName, landType );
        }
    }

    // TODO: Change to be able to call the parent function.
    // Right now doesn't work since two classes aren't derived from common parent.
    // To do this, likely need a     ILandAllocator::LandUsageType getLandType() function so as to
    // create the proper land leaf type.
    
    Technology::completeInit( aRegionName, aSectorName, aDepFinder, aSubsectorInfo,
                              aLandAllocator, aGlobalTechDB );

    // Store away the land allocator.
    mLandAllocator = aLandAllocator;

    // Set rotation period variable so this can be used throughout object
    mRotationPeriod = aSubsectorInfo->getInteger( "rotationPeriod", true );

    // Setup the land usage for this production.
    int techPeriod = scenario->getModeltime()->getyr_to_per( year );
    mLandAllocator->addLandUsage( landType, mName, ILandAllocator::eForest, techPeriod );

    setCalLandValues();
}

/*! \brief Sets calibrated land values to land allocator.
*
* This utility function is called twice. Once in completeInit so that initial
* shares can be set throughout the land allocator and again in initCalc()
* in case shares have been disrupted by a previous call to calc() (which is what
* currently happens in 1975).
*
* \author Steve Smith
*/
void ForestProductionTechnology::setCalLandValues() {
    const Modeltime* modeltime = scenario->getModeltime();
    int timestep = modeltime->gettimestep( modeltime->getyr_to_per(year));
    int nRotPeriodSteps = mRotationPeriod / timestep;

    // -1 means not read in
    if ( mCalValue.get() && ( calYield != -1 )) {
        calObservedYield = 0;     //Yield per year
        double calObservedLandUsed = 0;  //Land harvested per period as opposed to per step-periods in calLandUsed
        double calProductionTemp = mCalValue->getCalOutput( 1 );
        double calYieldTemp = calYield;
        int period = modeltime->getyr_to_per(year);
        if ( !mFutureProduction.isInited() ) {
            nRotPeriodSteps = 0;
        }

        // Loop through to set current and future land and production from forests.
        for ( int i = period; i <= period + nRotPeriodSteps; i++ ) {
            // Need to do be able to somehow get productivity change from other
            // periods. Or demand that productivity change is the same for all
            // calibration periods (could test in applyAgProdChange)
            if ( i > period ) {
                calProductionTemp += ( mFutureProduction - mCalValue->getCalOutput( 1 ) ) / nRotPeriodSteps;
                calYieldTemp = calYield * pow( 1 + agProdChange, double( timestep * ( i - 1 ) ) );
            }

            calLandUsed = calProductionTemp / calYieldTemp;
            mLandAllocator->setCalLandAllocation( landType, mName, calLandUsed, i, period );
            mLandAllocator->setCalObservedYield( landType, mName, calYieldTemp, i );
            if ( i == period ) {
                calObservedYield = calYieldTemp;
            }
        }      
    }
}

/*!
* \brief Calculate unnormalized technology unnormalized shares.
* \details Since food and forestry technologies are profit based, they do not
*          directly calculate a share. Instead, their share of total supply is
*          determined by the sharing which occurs in the land allocator. To
*          facilitate this the technology sets the intrinsic rate for the land
*          use into the land allocator. The technology share itself is set to 1.
* \param aRegionName Region name.
* \param aSectorName Sector name, also the name of the product.
* \param aGDP Regional GDP container.
* \param aPeriod Model period.
* \return Technology share, always 1 for ForestProductionTechnologies.
* \author James Blackwood, Steve Smith
*/
double ForestProductionTechnology::calcShare( const string& aRegionName,
                                              const string& aSectorName,
                                              const GDP* aGDP,
                                              const int aPeriod ) const
{
    assert( mProductionState[ aPeriod ]->isNewInvestment() );

    // Forest production technologies are profit based, so the amount of output
    // they produce is independent of the share.
    return 1;
}

void ForestProductionTechnology::calcCost( const string& aRegionName,
                                           const string& aSectorName,
                                           const int aPeriod )
{
    if( !mProductionState[ aPeriod ]->isOperating() ){
        return;
    }

    // If yield is GCal/Ha and prices are $/GCal, then rental rate is $/Ha
    // Passing in rate as $/GCal and setIntrinsicRate will set it to  $/Ha.
    double profitRate = calcProfitRate( aRegionName, getFutureMarket( aSectorName ), aPeriod );
    mLandAllocator->setIntrinsicRate( aRegionName, landType, mName, profitRate, aPeriod );

    // Override costs to a non-zero value as the cost for a food production
    // technology is not used for the shares.
    mCosts[ aPeriod ] = 1;
}

/*! \brief Calculates the output of the technology.
* \details Calculates the amount of current forestry output based on the amount
*          of planted forestry land and it's yield. Forestry production
*          technologies are profit based and determine their supply
*          independently of the passed in subsector demand. However, since this
*          is a solved market, in equilibrium the sum of the production of
*          technologies within a sector will equal the demand for the sector.
*          For forestry this supply is fixed because trees were planted several
*          periods before. Since the supply is inelastic, demand must adjust to
*          reach equilibrium.
* \param aRegionName Region name.
* \param aSectorName Sector name, also the name of the product.
* \param aVariableDemand Subsector demand for output.
* \param aFixedOutputScaleFactor Fixed output scale factor.
* \param aGDP Regional GDP container.
* \param aPeriod Model period.
*/
void ForestProductionTechnology::production( const string& aRegionName,
                                             const string& aSectorName,
                                             const double aVariableDemand,
                                             const double aFixedOutputScaleFactor,
                                             const GDP* aGDP,
                                             const int aPeriod )
{
    if( !mProductionState[ aPeriod ]->isOperating() ){
            // Set physical output to zero.
        mOutputs[ 0 ]->setPhysicalOutput( 0, aRegionName,
                                          mCaptureComponent.get(),
                                          aPeriod );
        return;
    }

    // Calculate profit rate.
    double profitRate = calcProfitRate( aRegionName, getFutureMarket( aSectorName ), aPeriod );

    // Calculating the yield for future forest.
    const int harvestPeriod = getHarvestPeriod( aPeriod );
    mLandAllocator->calcYield( landType, mName, aRegionName,
                               profitRate, harvestPeriod, aPeriod );
    
    // Add the supply of future forestry to the future market.
    double futureSupply = calcSupply( aRegionName, aSectorName, harvestPeriod );
    Marketplace* marketplace = scenario->getMarketplace();
    marketplace->addToSupply( getFutureMarket( aSectorName ), aRegionName, futureSupply, aPeriod );

    // now calculate the amount to be consumed this period (ie. planted steps
    // periods ago).
    double primaryOutput = calcSupply( aRegionName, aSectorName, aPeriod );

    // Set the input to be the land used. TODO: Determine a way to improve this.
    // This would be wrong if the fuelname had an emissions coefficient, or if
    // there were a fuel or other input. When multiple inputs are complete there
    // should be a specific land input.
    mInput[ aPeriod ] = mLandAllocator->getLandAllocation( landType, mName, aPeriod );
    calcEmissionsAndOutputs( aRegionName, mInput[ aPeriod ], primaryOutput, aGDP, aPeriod );
}

/*! \brief Calculate the profit rate for the technology.
* \details Calculates the profit rate for the forestry technology. This is equal
*          to the net present value of the market price minus the variable cost 
*          Profit rate can be negative.
* \param aRegionName Region name.
* \param aProductName Name of the product for which to calculate the profit
*        rate. Must be an output of the technology.
* \return The profit rate.
*/
double ForestProductionTechnology::calcProfitRate( const string& aRegionName,
                                                   const string& aProductName,
                                                   const int aPeriod ) const
{
    // Calculate the future profit rate.
    // TODO: If a ForestProductionTechnology had emissions this would not be correct as the 
    // emissions cost would be calculated for the present year and the emissions would be 
    // charged in a future year.
    double profitRate = FoodProductionTechnology::calcProfitRate( aRegionName, aProductName, aPeriod );

    // Calculate the net present value.
    double netPresentValue = profitRate * calcDiscountFactor();

    return netPresentValue;
}

/*! \brief Calculate the factor which discounts the future value of the forest
*          harvest between the future harvest period and the current period and
*          levels across the number of years during which the trees are
*          grown.
* \return The discount factor.
*/
double ForestProductionTechnology::calcDiscountFactor() const {
    assert( mRotationPeriod > 0 );
    return interestRate / ( pow( 1 + interestRate, static_cast<int>( mRotationPeriod ) ) - 1 );
}

/*! \brief Get the period in which the crop will be harvested if planted in the
*          current period.
* \param aCurrentPeriod Current period.
* \return The harvest period.
*/
int ForestProductionTechnology::getHarvestPeriod( const int aCurrentPeriod ) const {
    const Modeltime* modeltime = scenario->getModeltime();
    return aCurrentPeriod + mRotationPeriod / modeltime->gettimestep( modeltime->getyr_to_per( year ) );
}

/*! \brief Get the future market for a given product name.
* \param aProductName Name of the product.
* \return Name of the future market.
*/
const string ForestProductionTechnology::getFutureMarket( const string& aProductName ) const {
    return "Future" + aProductName;
}
