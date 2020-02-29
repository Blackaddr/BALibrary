#ifndef DMASPI_H
#define DMASPI_H

#include <Arduino.h>
#include <util/atomic.h>

#if(!defined(__arm__) && defined(TEENSYDUINO))
  #error This library is for teensyduino 1.21 on Teensy 3.0, 3.1 and Teensy LC only.
#endif

#include <SPI.h>
#include "DMAChannel.h"
#include <core_pins.h>

//#include <core_cm7.h>
#include <arm_math.h>

//#define DEBUG_DMASPI 1


/** \brief Specifies the desired CS suppression
**/
enum TransferType
{
  NORMAL,      //*< The transfer will use CS at beginning and end **/
  NO_START_CS, //*< Skip the CS activation at the start **/
  NO_END_CS    //*< SKip the CS deactivation at the end **/
};

/** \brief An abstract base class that provides an interface for chip select classes.
**/
class AbstractChipSelect
{
	public:
    /** \brief Called to select a chip. The implementing class can do other things as well.
    **/
    virtual void select(TransferType transferType = TransferType::NORMAL) = 0;

    /** \brief Called to deselect a chip. The implementing class can do other things as well.
    **/
    virtual void deselect(TransferType transferType = TransferType::NORMAL) = 0;

    /** \brief the virtual destructor needed to inherit from this class **/
		virtual ~AbstractChipSelect() {}
};


/** \brief "do nothing" chip select class **/
class DummyChipSelect : public AbstractChipSelect
{
  void select(TransferType transferType = TransferType::NORMAL) override {}

  void deselect(TransferType transferType = TransferType::NORMAL) override {}
};

/** \brief "do nothing" chip select class that
 * outputs a message through Serial when something happens
**/
class DebugChipSelect : public AbstractChipSelect
{
  void select(TransferType transferType = TransferType::NORMAL) override {Serial.println("Debug CS: select()");}
  void deselect(TransferType transferType = TransferType::NORMAL) override {Serial.println("Debug CS: deselect()");}
};

/** \brief An active low chip select class. This also configures the given pin.
 * Warning: This class is hardcoded to manage a transaction on SPI (SPI0, that is).
 * If you want to use SPI1: Use AbstractChipSelect1 (see below)
 * If you want to use SPI2: Create AbstractChipSelect2 (adapt the implementation accordingly).
 * Something more flexible is on the way.
**/
class ActiveLowChipSelect : public AbstractChipSelect
{
  public:
    /** Configures a chip select pin for OUTPUT mode,
     * manages the chip selection and a corresponding SPI transaction
     *
     * The chip select pin is asserted \e after the SPI settings are applied
     * and deasserted before the SPI transaction ends.
     * \param pin the CS pin to use
     * \param settings which SPI settings to apply when the chip is selected
    **/
    ActiveLowChipSelect(const unsigned int& pin, const SPISettings& settings)
      : pin_(pin),
      settings_(settings)
    {
      pinMode(pin, OUTPUT);
      digitalWriteFast(pin, 1);
    }

    /** \brief begins an SPI transaction selects the chip (sets the pin to low) and
    **/
    void select(TransferType transferType = TransferType::NORMAL) override
    {
      SPI.beginTransaction(settings_);
      if (transferType == TransferType::NO_START_CS) {
    	  return;
      }
      digitalWriteFast(pin_, 0);
    }

    /** \brief deselects the chip (sets the pin to high) and ends the SPI transaction
    **/
    void deselect(TransferType transferType = TransferType::NORMAL) override
    {
      if (transferType == TransferType::NO_END_CS) {
      } else {
    	  digitalWriteFast(pin_, 1);
      }
      SPI.endTransaction();
    }
  private:
    const unsigned int pin_;
    const SPISettings settings_;

};

#if defined(__MK66FX1M0__)
class ActiveLowChipSelect1 : public AbstractChipSelect
{
  public:
    /** Equivalent to AbstractChipSelect, but for SPI1.
    **/
    ActiveLowChipSelect1(const unsigned int& pin, const SPISettings& settings)
      : pin_(pin),
      settings_(settings)
    {
      pinMode(pin, OUTPUT);
      digitalWriteFast(pin, 1);
    }

    /** \brief begins an SPI transaction selects the chip (sets the pin to low) and
    **/
    void select(TransferType transferType = TransferType::NORMAL) override
    {
      SPI1.beginTransaction(settings_);
      if (transferType == TransferType::NO_START_CS) {
        return;
      }
      digitalWriteFast(pin_, 0);
    }

    /** \brief deselects the chip (sets the pin to high) and ends the SPI transaction
    **/
    void deselect(TransferType transferType = TransferType::NORMAL) override
    {
      if (transferType == TransferType::NO_END_CS) {
      } else {
        digitalWriteFast(pin_, 1);
      }
      SPI1.endTransaction();
    }
  private:
    const unsigned int pin_;
    const SPISettings settings_;

};
#endif



#if defined(DEBUG_DMASPI)
  #define DMASPI_PRINT(x) do {Serial.printf x ; Serial.flush();} while (0);
#else
  #define DMASPI_PRINT(x) do {} while (0);
#endif

namespace DmaSpi
{
  /** \brief describes an SPI transfer
   *
   * Transfers are kept in a queue (intrusive linked list) until they are processed by the DmaSpi driver.
   *
  **/
  class Transfer
  {
    public:
      /** \brief The Transfer's current state.
      *
      **/
      enum State
      {
        idle, /**< The Transfer is idle, the DmaSpi has not seen it yet. **/
        eDone, /**< The Transfer is done. **/
        pending, /**< Queued, but not handled yet. **/
        inProgress, /**< The DmaSpi driver is currently busy executing this Transfer. **/
        error /**< An error occured. **/
      };

      /** \brief Creates a Transfer object.
      * \param pSource pointer to the data source. If this is nullptr, the fill value is used instead.
      * \param transferCount the number of SPI transfers to perform.
      * \param pDest pointer to the data sink. If this is nullptr, data received from the slave will be discarded.
      * \param fill if pSource is nullptr, this value is sent to the slave instead.
      * \param cs pointer to a chip select object.
      *   If not nullptr, cs->select() is called when the Transfer is started and cs->deselect() is called when the Transfer is finished.
      **/
      Transfer(const uint8_t* pSource = nullptr,
                  const uint16_t& transferCount = 0,
                  volatile uint8_t* pDest = nullptr,
                  const uint8_t& fill = 0,
                  AbstractChipSelect* cs = nullptr,
                  TransferType transferType = TransferType::NORMAL,
                  uint8_t *pSourceIntermediate = nullptr,
                  volatile uint8_t *pDestIntermediate   = nullptr
      ) : m_state(State::idle),
        m_pSource(pSource),
        m_transferCount(transferCount),
        m_pDest(pDest),
        m_fill(fill),
        m_pNext(nullptr),
        m_pSelect(cs),
        m_transferType(transferType),
        m_pSourceIntermediate(pSourceIntermediate),
        m_pDestIntermediate(pDestIntermediate)
      {
          DMASPI_PRINT(("Transfer @ %p\n", this));
      };

      /** \brief Check if the Transfer is busy, i.e. may not be modified.
      **/
      bool busy() const {return ((m_state == State::pending) || (m_state == State::inProgress) || (m_state == State::error));}

      /** \brief Check if the Transfer is done.
      **/
      bool done() const {return (m_state == State::eDone);}

//      private:
      volatile State m_state;
      const uint8_t* m_pSource;
      uint16_t m_transferCount;
      volatile uint8_t* m_pDest;
      uint8_t m_fill;
      Transfer* m_pNext;
      AbstractChipSelect* m_pSelect;
      TransferType m_transferType;

      uint8_t *m_pSourceIntermediate = nullptr;
      volatile uint8_t *m_pDestIntermediate   = nullptr;
      volatile uint8_t *m_pDestOriginal       = nullptr;
  };
} // namespace DmaSpi


template<typename DMASPI_INSTANCE, typename SPICLASS, SPICLASS& m_Spi>
class AbstractDmaSpi
{
  public:
    using Transfer = DmaSpi::Transfer;

   /** \brief arduino-style initialization.
     *
     * During initialization, two DMA channels are allocated. If that fails, this function returns false.
     * If the channels could be allocated, those DMA channel fields that don't change during DMA SPI operation
     * are initialized to the values they will have at runtime.
     *
     * \return true if initialization was successful; false otherwise.
     * \see end()
    **/
    static bool begin()
    {
      if(init_count_ > 0)
      {
        return true; // this is not particularly bad, so we can return true
      }
      init_count_++;
      DMASPI_PRINT(("DmaSpi::begin() : "));
      // create DMA channels, might fail
      if (!createDmaChannels())
      {
        DMASPI_PRINT(("could not create DMA channels\n"));
        return false;
      }
      state_ = eStopped;
      // tx: known destination (SPI), no interrupt, finish silently
      begin_setup_txChannel();
      if (txChannel_()->error())
      {
        destroyDmaChannels();
        DMASPI_PRINT(("tx channel error\n"));
        return false;
      }

      // rx: known source (SPI), interrupt on completion
      begin_setup_rxChannel();
      if (rxChannel_()->error())
      {
        destroyDmaChannels();
        DMASPI_PRINT(("rx channel error\n"));
        return false;
      }

      return true;
    }

    static void begin_setup_txChannel() {DMASPI_INSTANCE::begin_setup_txChannel_impl();}
    static void begin_setup_rxChannel() {DMASPI_INSTANCE::begin_setup_rxChannel_impl();}

    /** \brief Allow the DMA SPI to start handling Transfers. This must be called after begin().
     * \see running()
     * \see busy()
     * \see stop()
     * \see stopping()
     * \see stopped()
    **/
    static void start()
    {
      DMASPI_PRINT(("DmaSpi::start() : state_ = "));
      switch(state_)
      {
        case eStopped:
          DMASPI_PRINT(("eStopped\n"));
          state_ = eRunning;
          beginPendingTransfer();
          break;

        case eRunning:
          DMASPI_PRINT(("eRunning\n"));
          break;

        case eStopping:
          DMASPI_PRINT(("eStopping\n"));
          state_ = eRunning;
          break;

        default:
          DMASPI_PRINT(("unknown\n"));
          state_ = eError;
          break;
      }
    }

    /** \brief check if the DMA SPI is in running state.
     * \return true if the DMA SPI is in running state, false otherwise.
     * \see start()
     * \see busy()
     * \see stop()
     * \see stopping()
     * \see stopped()
    **/
    static bool running() {return state_ == eRunning;}

    /** \brief register a Transfer to be handled by the DMA SPI.
     * \return false if the Transfer had an invalid transfer count (zero or greater than 32767), true otherwise.
     * \post the Transfer state is Transfer::State::pending, or Transfer::State::error if the transfer count was invalid.
    **/
    static bool registerTransfer(Transfer& transfer)
    {
      DMASPI_PRINT(("DmaSpi::registerTransfer(%p)\n", &transfer));
      if ((transfer.busy())
       || (transfer.m_transferCount == 0) // no zero length transfers allowed
       || (transfer.m_transferCount >= 0x8000)) // max CITER/BITER count with ELINK = 0 is 0x7FFF, so reject
      {
        DMASPI_PRINT(("  Transfer is busy or invalid, dropped\n"));
        transfer.m_state = Transfer::State::error;
        return false;
      }
      addTransferToQueue(transfer);
      if ((state_ == eRunning) && (!busy()))
      {
        DMASPI_PRINT(("  starting transfer\n"));
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
        {
          beginPendingTransfer();
        }
      }
      return true;
    }


    /** \brief Check if the DMA SPI is busy, which means that it is currently handling a Transfer.
     \return true if a Transfer is being handled.
     * \see start()
     * \see running()
     * \see stop()
     * \see stopping()
     * \see stopped()
    **/
    static bool busy()
    {
      return (m_pCurrentTransfer != nullptr);
    }

    /** \brief Request the DMA SPI to stop handling Transfers.
     *
     * The stopping driver may finish a current Transfer, but it will then not start a new, pending one.
     * \see start()
     * \see running()
     * \see busy()
     * \see stopping()
     * \see stopped()
    **/
    static void stop()
    {
      ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
      {
        switch(state_)
        {
          case eStopped:
            break;
          case eRunning:
            if (busy())
            {
              state_ = eStopping;
            }
            else
            {
              // this means that the DMA SPI simply has nothing to do
              state_ = eStopped;
            }
            break;
          case eStopping:
            break;
          default:
            state_ = eError;
            break;
        }
      }
    }

    /** \brief See if the DMA SPI is currently switching from running to stopped state
     * \return true if the DMA SPI is switching from running to stopped state
     * \see start()
     * \see running()
     * \see busy()
     * \see stop()
     * \see stopped()
    **/
    static bool stopping() { return (state_ == eStopping); }

    /** \brief See if the DMA SPI is stopped
    * \return true if the DMA SPI is in stopped state, i.e. not handling pending Transfers
     * \see start()
     * \see running()
     * \see busy()
     * \see stop()
     * \see stopping()
    **/
    static bool stopped() { return (state_ == eStopped); }

    /** \brief Shut down the DMA SPI
     *
     * Deallocates DMA channels and sets the internal state to error (this might not be an intelligent name for that)
     * \see begin()
    **/
    static void end()
    {
      if (init_count_ == 0)
      {
        state_ = eError;
        return;
      }
      if (init_count_ == 1)
      {
        init_count_--;
        destroyDmaChannels();
        state_ = eError;
        return;
      }
      else
      {
        init_count_--;
        return;
      }
    }

    /** \brief get the last value that was read from a slave, but discarded because the Transfer didn't specify a sink
    **/
    static uint8_t devNull()
    {
      return m_devNull;
    }

  protected:
    enum EState
    {
      eStopped,
      eRunning,
      eStopping,
      eError
    };

    static void addTransferToQueue(Transfer& transfer)
    {
      transfer.m_state = Transfer::State::pending;
      transfer.m_pNext = nullptr;
      DMASPI_PRINT(("  DmaSpi::addTransferToQueue() : queueing transfer\n"));
      ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
      {
        if (m_pNextTransfer == nullptr)
        {
          m_pNextTransfer = &transfer;
        }
        else
        {
          m_pLastTransfer->m_pNext = &transfer;
        }
        m_pLastTransfer = &transfer;
      }
    }

    static void post_finishCurrentTransfer() {DMASPI_INSTANCE::post_finishCurrentTransfer_impl();}

    // finishCurrentTransfer is called from rxISR_()
    static void finishCurrentTransfer()
    {
        DMASPI_PRINT((" inside finishCurrentTransfer()\n"));
      if (m_pCurrentTransfer->m_pSelect != nullptr)
      {
        m_pCurrentTransfer->m_pSelect->deselect(m_pCurrentTransfer->m_transferType);
      }
      else
      {
        m_Spi.endTransaction();
      }
      m_pCurrentTransfer->m_state = Transfer::State::eDone;
      DMASPI_PRINT(("  finishCurrentTransfer() @ %p\n", m_pCurrentTransfer));
      m_pCurrentTransfer = nullptr;
      post_finishCurrentTransfer();
    }

    static bool createDmaChannels()
    {
      if (txChannel_() == nullptr)
      {
        return false;
      }
      if (rxChannel_() == nullptr)
      {
        delete txChannel_();
        return false;
      }
      return true;
    }

    static void destroyDmaChannels()
    {
      if (rxChannel_() != nullptr)
      {
        delete rxChannel_();
      }
      if (txChannel_() != nullptr)
      {
        delete txChannel_();
      }
    }

    static DMAChannel* rxChannel_()
    {
      static DMAChannel* pChannel = new DMAChannel();
      return pChannel;
    }

    static DMAChannel* txChannel_()
    {
      static DMAChannel* pChannel = new DMAChannel();
      return pChannel;
    }

    static void rxIsr_()
    {
      DMASPI_PRINT(("DmaSpi::rxIsr_()\n"));
      rxChannel_()->clearInterrupt();
      // end current transfer: deselect and mark as done

      // Check if intermediate buffer was used
      if (m_pCurrentTransfer->m_pDestIntermediate) {
          // copy when using an intermediate buffer
          memcpy((void *)m_pCurrentTransfer->m_pDestOriginal, // DMA contents copied to original
                 (void *)m_pCurrentTransfer->m_pDest, // source is the actual DMA buffer
                 m_pCurrentTransfer->m_transferCount);
      }
      finishCurrentTransfer();

      DMASPI_PRINT(("  state = "));
      switch(state_)
      {
        case eStopped: // this should not happen!
        DMASPI_PRINT(("eStopped\n"));
          state_ = eError;
          break;
        case eRunning:
          DMASPI_PRINT(("eRunning\n"));
          beginPendingTransfer();
          break;
        case eStopping:
          DMASPI_PRINT(("eStopping\n"));
          state_ = eStopped;
          break;
        case eError:
          DMASPI_PRINT(("eError\n"));
          break;
        default:
          DMASPI_PRINT(("eUnknown\n"));
          state_ = eError;
          break;
      }
    }

    static void pre_cs() {DMASPI_INSTANCE::pre_cs_impl();}
    static void post_cs() {DMASPI_INSTANCE::post_cs_impl();}

    static void beginPendingTransfer()
    {
      if (m_pNextTransfer == nullptr)
      {
        DMASPI_PRINT(("DmaSpi::beginPendingTransfer: no pending transfer\n"));
        return;
      }

      m_pCurrentTransfer = m_pNextTransfer;
      DMASPI_PRINT(("DmaSpi::beginPendingTransfer: starting transfer @ %p\n", m_pCurrentTransfer));
      m_pCurrentTransfer->m_state = Transfer::State::inProgress;
      m_pNextTransfer = m_pNextTransfer->m_pNext;
      if (m_pNextTransfer == nullptr)
      {
        DMASPI_PRINT(("  this was the last in the queue\n"));
        m_pLastTransfer = nullptr;
      }

      // configure Rx DMA
      if (m_pCurrentTransfer->m_pDest != nullptr)
      {
        // real data sink
        DMASPI_PRINT(("  real sink\n"));

        // Check for intermediate buffer
        if (m_pCurrentTransfer->m_pDestIntermediate) {
            // Modify the DMA so it will fill the intermediate buffer instead
            // store the original buffer for memcpy in rx_isr()
            m_pCurrentTransfer->m_pDestOriginal = m_pCurrentTransfer->m_pDest;
            m_pCurrentTransfer->m_pDest = m_pCurrentTransfer->m_pDestIntermediate;
        }
        arm_dcache_flush_delete((void *)m_pCurrentTransfer->m_pDest, m_pCurrentTransfer->m_transferCount);
        rxChannel_()->destinationBuffer(m_pCurrentTransfer->m_pDest,
                                        m_pCurrentTransfer->m_transferCount);
      }
      else
      {
        // dummy data sink
        DMASPI_PRINT(("  dummy sink\n"));
        rxChannel_()->destination(m_devNull);
        rxChannel_()->transferCount(m_pCurrentTransfer->m_transferCount);
      }

      // configure Tx DMA
      if (m_pCurrentTransfer->m_pSource != nullptr)
      {
        // real data source
        if (m_pCurrentTransfer->m_pSourceIntermediate) {
            // copy and use the intermediate buffer
            memcpy((void*)m_pCurrentTransfer->m_pSourceIntermediate,
                   (void*)m_pCurrentTransfer->m_pSource,
                    m_pCurrentTransfer->m_transferCount
                  );
            // DMA will now transfer from intermediate buffer
            m_pCurrentTransfer->m_pSource = m_pCurrentTransfer->m_pSourceIntermediate;
        }
        DMASPI_PRINT(("  real source\n"));
        arm_dcache_flush_delete((void *)m_pCurrentTransfer->m_pSource, m_pCurrentTransfer->m_transferCount);
        txChannel_()->sourceBuffer(m_pCurrentTransfer->m_pSource,
                                   m_pCurrentTransfer->m_transferCount);
      }
      else
      {
        // dummy data source
        DMASPI_PRINT(("  dummy source\n"));
        txChannel_()->source(m_pCurrentTransfer->m_fill);
        txChannel_()->transferCount(m_pCurrentTransfer->m_transferCount);
      }

      DMASPI_PRINT(("calling pre_cs() "));
      pre_cs();

      // Select Chip
      if (m_pCurrentTransfer->m_pSelect != nullptr)
      {
        m_pCurrentTransfer->m_pSelect->select(m_pCurrentTransfer->m_transferType);
      }
      else
      {
        m_Spi.beginTransaction(SPISettings());
      }

      DMASPI_PRINT(("calling post_cs() "));
      post_cs();
    }

    static size_t init_count_;
    static volatile EState state_;
    static Transfer* volatile m_pCurrentTransfer;
    static Transfer* volatile m_pNextTransfer;
    static Transfer* volatile m_pLastTransfer;
    static volatile uint8_t m_devNull;
    //static SPICLASS& m_Spi;
};

template<typename DMASPI_INSTANCE, typename SPICLASS, SPICLASS& m_Spi>
size_t AbstractDmaSpi<DMASPI_INSTANCE, SPICLASS, m_Spi>::init_count_ = 0;

template<typename DMASPI_INSTANCE, typename SPICLASS, SPICLASS& m_Spi>
volatile typename AbstractDmaSpi<DMASPI_INSTANCE, SPICLASS, m_Spi>::EState AbstractDmaSpi<DMASPI_INSTANCE, SPICLASS, m_Spi>::state_ = eError;

template<typename DMASPI_INSTANCE, typename SPICLASS, SPICLASS& m_Spi>
typename AbstractDmaSpi<DMASPI_INSTANCE, SPICLASS, m_Spi>::Transfer* volatile AbstractDmaSpi<DMASPI_INSTANCE, SPICLASS, m_Spi>::m_pNextTransfer = nullptr;

template<typename DMASPI_INSTANCE, typename SPICLASS, SPICLASS& m_Spi>
typename AbstractDmaSpi<DMASPI_INSTANCE, SPICLASS, m_Spi>::Transfer* volatile AbstractDmaSpi<DMASPI_INSTANCE, SPICLASS, m_Spi>::m_pCurrentTransfer = nullptr;

template<typename DMASPI_INSTANCE, typename SPICLASS, SPICLASS& m_Spi>
typename AbstractDmaSpi<DMASPI_INSTANCE, SPICLASS, m_Spi>::Transfer* volatile AbstractDmaSpi<DMASPI_INSTANCE, SPICLASS, m_Spi>::m_pLastTransfer = nullptr;

template<typename DMASPI_INSTANCE, typename SPICLASS, SPICLASS& m_Spi>
volatile uint8_t AbstractDmaSpi<DMASPI_INSTANCE, SPICLASS, m_Spi>::m_devNull = 0;

//void dump_dma(DMAChannel *dmabc)
//{
//    Serial.printf("%x %x:", (uint32_t)dmabc, (uint32_t)dmabc->TCD);
//
//    Serial.printf("SA:%x SO:%d AT:%x NB:%x SL:%d DA:%x DO: %d CI:%x DL:%x CS:%x BI:%x\n", (uint32_t)dmabc->TCD->SADDR,
//        dmabc->TCD->SOFF, dmabc->TCD->ATTR, dmabc->TCD->NBYTES, dmabc->TCD->SLAST, (uint32_t)dmabc->TCD->DADDR,
//        dmabc->TCD->DOFF, dmabc->TCD->CITER, dmabc->TCD->DLASTSGA, dmabc->TCD->CSR, dmabc->TCD->BITER);
//    Serial.flush();
//}

#if defined(__IMXRT1062__) // T4.0

class DmaSpi0 : public AbstractDmaSpi<DmaSpi0, SPIClass, SPI>
{
public:
  static void begin_setup_txChannel_impl()
  {
    txChannel_()->disable();
    txChannel_()->destination((volatile uint8_t&)IMXRT_LPSPI4_S.TDR);
    txChannel_()->disableOnCompletion();
    txChannel_()->triggerAtHardwareEvent(DMAMUX_SOURCE_LPSPI4_TX);
    //txChannel_()->triggerAtTransfersOf(*rxChannel_);
  }

  static void begin_setup_rxChannel_impl()
  {
    rxChannel_()->disable();
    rxChannel_()->source((volatile uint8_t&)IMXRT_LPSPI4_S.RDR); // POPR is the receive fifo register for the SPI
    rxChannel_()->disableOnCompletion();
    rxChannel_()->triggerAtHardwareEvent(DMAMUX_SOURCE_LPSPI4_RX); // The DMA RX id for MT66 is 14
    rxChannel_()->attachInterrupt(rxIsr_);
    rxChannel_()->interruptAtCompletion();

  }

  static void pre_cs_impl()
  {
    if (LPSPI4_SR & 0x1800) {
        DMASPI_PRINT(("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!ERROR SR reg is %08X\n", LPSPI4_SR));
    }
    DMASPI_PRINT(("********************************************CHECK SR reg is %08X\n", LPSPI4_SR));

    IMXRT_LPSPI4_S.TCR = (IMXRT_LPSPI4_S.TCR & ~(LPSPI_TCR_FRAMESZ(31))) | LPSPI_TCR_FRAMESZ(7);
    IMXRT_LPSPI4_S.FCR = 0;

    //IMXRT_LPSPI4_S.CR = LPSPI_CR_MEN | LPSPI_CR_RRF | LPSPI_CR_RTF;
    IMXRT_LPSPI4_S.CR = LPSPI_CR_MEN; // I had to add the enable otherwise it wont' work

    // Lets try to output the first byte to make sure that we are in 8 bit mode...
    IMXRT_LPSPI4_S.DER = LPSPI_DER_TDDE | LPSPI_DER_RDDE; //enable DMA on both TX and RX
    IMXRT_LPSPI4_S.SR = 0x3f00; // clear out all of the other status...

//    if (m_pCurrentTransfer->m_pSource) {
//        arm_dcache_flush((void *)m_pCurrentTransfer->m_pSource, m_pCurrentTransfer->m_transferCount);
//    }

  }

//  static void pre_cs_impl()
//  {
//
//      //LPSPI4_PARAM = LPSPI4_PARAM;
//      //LPSPI4_PARAM = 0x0404;
//      //DMASPI_PRINT(("!!!!!!!!!!!!!!!!!!!!!PARAM reg is %08X\n", LPSPI4_PARAM));
//      txChannel_()->TCD->ATTR_SRC = 0;      //Make sure set for 8 bit mode...
//      txChannel_()->TCD->SLAST = 0;   // Finish with it pointing to next location
//      rxChannel_()->TCD->ATTR_DST = 0;      //Make sure set for 8 bit mode...
//      rxChannel_()->TCD->DLASTSGA = 0;
//
//      //DMASPI_PRINT(("STATUS SR reg is %08X\n", LPSPI4_SR));
//      if (LPSPI4_SR & 0x1800) {
//          DMASPI_PRINT(("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!ERROR SR reg is %08X\n", LPSPI4_SR));
//      }
//      LPSPI4_SR = 0x3f00; // clear various error and status flags
//      DMASPI_PRINT(("********************************************CHECK SR reg is %08X\n", LPSPI4_SR));
//
//      LPSPI4_TCR = (LPSPI4_TCR & ~(LPSPI_TCR_FRAMESZ(31))) | LPSPI_TCR_FRAMESZ(7); // Set the FRAMESZ to 7 for 8-bit frame size
//      LPSPI4_FCR = 0; // set watermarks to zero, this ensures ready flag is set whenever fifo is not empty
//
//      LPSPI4_CR = LPSPI_CR_MEN | LPSPI_CR_RRF | LPSPI_CR_RTF; //enable module and reset both FIFOs
//      LPSPI4_DER = LPSPI_DER_TDDE | LPSPI_DER_RDDE; // enable DMA on both TX and RX
//  }

  static void post_cs_impl()
  {
    rxChannel_()->enable();
    txChannel_()->enable();
    DMASPI_PRINT(("Done post_cs_impl()\n"));
  }

  static void post_finishCurrentTransfer_impl()
  {
    IMXRT_LPSPI4_S.FCR = LPSPI_FCR_TXWATER(15); // _spi_fcr_save; // restore the FSR status...
    IMXRT_LPSPI4_S.DER = 0;   // DMA no longer doing TX (or RX)

    IMXRT_LPSPI4_S.CR = LPSPI_CR_MEN | LPSPI_CR_RRF | LPSPI_CR_RTF;   // actually clear both...
    IMXRT_LPSPI4_S.SR = 0x3f00; // clear out all of the other status...

//    if (m_pCurrentTransfer->m_pDest) {
//        arm_dcache_delete((void *)m_pCurrentTransfer->m_pDest, m_pCurrentTransfer->m_transferCount);
//    }
  }

//  static void post_finishCurrentTransfer_impl()
//  {
//    //LPSPI4_FCR = LPSPI_FCR_TXWATER(15); // restore FSR status
//    LPSPI4_DER = 0; // DMA no longer doing TX or RX
//    LPSPI4_CR = LPSPI_CR_MEN | LPSPI_CR_RRF | LPSPI_CR_RTF; //enable module and reset both FIFOs
//    LPSPI4_SR = 0x3f00; // clear out all the other statuses
//  }

private:
};

extern DmaSpi0 DMASPI0;

#elif defined(KINETISK)

class DmaSpi0 : public AbstractDmaSpi<DmaSpi0, SPIClass, SPI>
{
public:
  static void begin_setup_txChannel_impl()
  {
    txChannel_()->disable();
    txChannel_()->destination((volatile uint8_t&)SPI0_PUSHR); // PUSHR is the transmit fifo register for the SPI
    txChannel_()->disableOnCompletion();
    txChannel_()->triggerAtHardwareEvent(DMAMUX_SOURCE_SPI0_TX); // The DMA TX id for MT66 is 15
  }

  static void begin_setup_rxChannel_impl()
  {
    rxChannel_()->disable();
    rxChannel_()->source((volatile uint8_t&)SPI0_POPR); // POPR is the receive fifo register for the SPI
    rxChannel_()->disableOnCompletion();
    rxChannel_()->triggerAtHardwareEvent(DMAMUX_SOURCE_SPI0_RX); // The DMA RX id for MT66 is 14
    rxChannel_()->attachInterrupt(rxIsr_);
    rxChannel_()->interruptAtCompletion();
  }

  static void pre_cs_impl()
  {
    SPI0_SR = 0xFF0F0000; // Clear various flags including Transfer complete, TXRX Status, End of Queue, Transmit FIFO underflow, Transmit FIFO Fill, Rx FIFO overflow, Rx fifo drain

    // Request Select Enable Register
    // RFDF_RE Rx fifo drain request enable, enables the RFDF flag in SPI0_SR
    // RFDF_DIRS Rx fifo drain selects DMA request instead of interrupt request
    // TFFF_RE Transmit Fifo fill request enable
    // TFFF_DIRS Transmit fifo fill selct DMA instead of interrupt
    SPI0_RSER = SPI_RSER_RFDF_RE | SPI_RSER_RFDF_DIRS | SPI_RSER_TFFF_RE | SPI_RSER_TFFF_DIRS;
  }

  static void post_cs_impl()
  {
    rxChannel_()->enable();
    txChannel_()->enable();
  }

  static void post_finishCurrentTransfer_impl()
  {
    SPI0_RSER = 0; //DSPI DMA/Interrupt Request Select and Enable Register
    SPI0_SR = 0xFF0F0000; // DSPI status register clear flags, same as above
  }

private:
};

extern DmaSpi0 DMASPI0;


#if defined(__MK66FX1M0__)

class DmaSpi1 : public AbstractDmaSpi<DmaSpi1, SPIClass, SPI1>
{
public:
  static void begin_setup_txChannel_impl()
  {
    txChannel_()->disable();
    txChannel_()->destination((volatile uint8_t&)SPI1_PUSHR);
    txChannel_()->disableOnCompletion();
    txChannel_()->triggerAtHardwareEvent(DMAMUX_SOURCE_SPI1_TX);
  }

  static void begin_setup_rxChannel_impl()
  {
    rxChannel_()->disable();
    rxChannel_()->source((volatile uint8_t&)SPI1_POPR);
    rxChannel_()->disableOnCompletion();
    rxChannel_()->triggerAtHardwareEvent(DMAMUX_SOURCE_SPI1_RX);
    rxChannel_()->attachInterrupt(rxIsr_);
    rxChannel_()->interruptAtCompletion();
  }

  static void pre_cs_impl()
  {
    SPI1_SR = 0xFF0F0000;
    SPI1_RSER = SPI_RSER_RFDF_RE | SPI_RSER_RFDF_DIRS | SPI_RSER_TFFF_RE | SPI_RSER_TFFF_DIRS;
  }

  static void post_cs_impl()
  {
    rxChannel_()->enable();
    txChannel_()->enable();
  }

  static void post_finishCurrentTransfer_impl()
  {
    SPI1_RSER = 0;
    SPI1_SR = 0xFF0F0000;
  }

private:
};

/*
class DmaSpi2 : public AbstractDmaSpi<DmaSpi2, SPIClass, SPI2>
{
public:
  static void begin_setup_txChannel_impl()
  {
    txChannel_()->disable();
    txChannel_()->destination((volatile uint8_t&)SPI2_PUSHR);
    txChannel_()->disableOnCompletion();
    txChannel_()->triggerAtHardwareEvent(DMAMUX_SOURCE_SPI2_TX);
  }

  static void begin_setup_rxChannel_impl()
  {
    rxChannel_()->disable();
    rxChannel_()->source((volatile uint8_t&)SPI2_POPR);
    rxChannel_()->disableOnCompletion();
    rxChannel_()->triggerAtHardwareEvent(DMAMUX_SOURCE_SPI2_RX);
    rxChannel_()->attachInterrupt(rxIsr_);
    rxChannel_()->interruptAtCompletion();
  }

  static void pre_cs_impl()
  {
    SPI2_SR = 0xFF0F0000;
    SPI2_RSER = SPI_RSER_RFDF_RE | SPI_RSER_RFDF_DIRS | SPI_RSER_TFFF_RE | SPI_RSER_TFFF_DIRS;
  }

  static void post_cs_impl()
  {
    rxChannel_()->enable();
    txChannel_()->enable();
  }

  static void post_finishCurrentTransfer_impl()
  {
    SPI2_RSER = 0;
    SPI2_SR = 0xFF0F0000;
  }

private:
};
*/

extern DmaSpi1 DMASPI1;
//extern DmaSpi2 DMASPI2;
#endif // defined(__MK66FX1M0__)

#elif defined(KINETISL)
class DmaSpi0 : public AbstractDmaSpi<DmaSpi0, SPIClass, SPI>
{
public:
  static void begin_setup_txChannel_impl()
  {
    txChannel_()->disable();
    txChannel_()->destination((volatile uint8_t&)SPI0_DL);
    txChannel_()->disableOnCompletion();
    txChannel_()->triggerAtHardwareEvent(DMAMUX_SOURCE_SPI0_TX);
  }

  static void begin_setup_rxChannel_impl()
  {
    rxChannel_()->disable();
    rxChannel_()->source((volatile uint8_t&)SPI0_DL);
    rxChannel_()->disableOnCompletion();
    rxChannel_()->triggerAtHardwareEvent(DMAMUX_SOURCE_SPI0_RX);
    rxChannel_()->attachInterrupt(rxIsr_);
    rxChannel_()->interruptAtCompletion();
  }

  static void pre_cs_impl()
  {
    // disable SPI and enable SPI DMA requests
    SPI0_C1 &= ~(SPI_C1_SPE);
    SPI0_C2 |= SPI_C2_TXDMAE | SPI_C2_RXDMAE;
  }

  static void post_cs_impl()
  {
    rxChannel_()->enable();
    txChannel_()->enable();
  }

  static void post_finishCurrentTransfer_impl()
  {
    SPI0_C2 = 0;
    txChannel_()->clearComplete();
    rxChannel_()->clearComplete();
  }

private:
};

class DmaSpi1 : public AbstractDmaSpi<DmaSpi1, SPIClass, SPI1>
{
public:
  static void begin_setup_txChannel_impl()
  {
    txChannel_()->disable();
    txChannel_()->destination((volatile uint8_t&)SPI1_DL);
    txChannel_()->disableOnCompletion();
    txChannel_()->triggerAtHardwareEvent(DMAMUX_SOURCE_SPI1_TX);
  }

  static void begin_setup_rxChannel_impl()
  {
    rxChannel_()->disable();
    rxChannel_()->source((volatile uint8_t&)SPI1_DL);
    rxChannel_()->disableOnCompletion();
    rxChannel_()->triggerAtHardwareEvent(DMAMUX_SOURCE_SPI1_RX);
    rxChannel_()->attachInterrupt(rxIsr_);
    rxChannel_()->interruptAtCompletion();
  }

  static void pre_cs_impl()
  {
    // disable SPI and enable SPI DMA requests
    SPI1_C1 &= ~(SPI_C1_SPE);
    SPI1_C2 |= SPI_C2_TXDMAE | SPI_C2_RXDMAE;
  }

//  static void dumpCFG(const char *sz, uint32_t* p)
//  {
//    DMASPI_PRINT(("%s: %x %x %x %x \n", sz, p[0], p[1], p[2], p[3]));
//  }
  
  static void post_cs_impl()
  {
    DMASPI_PRINT(("post_cs S C1 C2: %x %x %x\n", SPI1_S, SPI1_C1, SPI1_C2));
//    dumpCFG("RX", (uint32_t*)(void*)rxChannel_()->CFG);
//    dumpCFG("TX", (uint32_t*)(void*)txChannel_()->CFG);
    rxChannel_()->enable();
    txChannel_()->enable();
  }

  static void post_finishCurrentTransfer_impl()
  {
    SPI1_C2 = 0;
    txChannel_()->clearComplete();
    rxChannel_()->clearComplete();
  }
private:
};

extern DmaSpi0 DMASPI0;
extern DmaSpi1 DMASPI1;

#else

#error Unknown chip

#endif // KINETISK else KINETISL

class DmaSpiGeneric
{
public:
	using Transfer = DmaSpi::Transfer;

	DmaSpiGeneric() {
		m_spiDma0 = &DMASPI0;
#if defined(__MK66FX1M0__)
		m_spiDma1 = &DMASPI1;
#endif
	}
	DmaSpiGeneric(int spiId) : DmaSpiGeneric() {
		m_spiSelect = spiId;
	}

	bool begin () {
		switch(m_spiSelect) {
		case 1 : return m_spiDma1->begin();
		default :
			return m_spiDma0->begin();
		}
	}

	void start () {
		switch(m_spiSelect) {
		case 1 : m_spiDma1->start(); return;
		default :
			m_spiDma0->start(); return;
		}
	}

	bool running () {
		switch(m_spiSelect) {
		case 1 : return m_spiDma1->running();
		default :
			return m_spiDma0->running();
		}
	}

	bool registerTransfer (Transfer& transfer) {
		switch(m_spiSelect) {
		case 1 : return m_spiDma1->registerTransfer(transfer);
		default :
			return m_spiDma0->registerTransfer(transfer);
		}
	}


	bool busy () {
		switch(m_spiSelect) {
		case 1 : return m_spiDma1->busy();
		default :
			return m_spiDma0->busy();
		}
	}

	void stop () {
		switch(m_spiSelect) {
		case 1 : m_spiDma1->stop(); return;
		default :
			m_spiDma0->stop(); return;
		}
	}

	bool stopping () {
		switch(m_spiSelect) {
		case 1 : return m_spiDma1->stopping();
		default :
			return m_spiDma0->stopping();
		}
	}

	bool stopped () {
		switch(m_spiSelect) {
		case 1 : return m_spiDma1->stopped();
		default :
			return m_spiDma0->stopped();
		}
	}

	void end () {
		switch(m_spiSelect) {
		case 1 : m_spiDma1->end(); return;
		default :
			m_spiDma0->end(); return;
		}
	}

	uint8_t devNull () {
		switch(m_spiSelect) {
		case 1 : return m_spiDma1->devNull();
		default :
			return m_spiDma0->devNull();
		}
	}

private:
	int m_spiSelect = 0;
	DmaSpi0 *m_spiDma0 = nullptr;
#if defined(__MK66FX1M0__)
	DmaSpi1 *m_spiDma1 = nullptr;
#else
	// just make it Spi0 so it compiles atleast
	DmaSpi0 *m_spiDma1 = nullptr;
#endif

};


#endif // DMASPI_H
