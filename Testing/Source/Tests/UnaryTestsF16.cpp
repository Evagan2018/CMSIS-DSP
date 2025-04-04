#include "UnaryTestsF16.h"
#include <stdio.h>
#include "Error.h"

#define SNR_THRESHOLD 57

/* 

Reference patterns are generated with
a double precision computation.

*/
#define REL_ERROR (1.1e-3)
#define ABS_ERROR (1.1e-3)

/*

Comparisons for QR

*/

#define SNR_QR_THRESHOLD 20
#define REL_QR_ERROR (3.0e-2)
#define ABS_QR_ERROR (3.0e-2)

/*

Comparisons for inverse

*/

/* Not very accurate for big matrix.
But big matrix needed for checking the vectorized code */

#define SNR_THRESHOLD_INV 51
#define REL_ERROR_INV (3.0e-3)
#define ABS_ERROR_INV (2.0e-2)

#define REL_ERROR_SOLVE (6.0e-2)
#define ABS_ERROR_SOLVE (2.0e-2)

/*

Comparison for Cholesky

*/
#define SNR_THRESHOLD_CHOL 45
#define REL_ERROR_CHOL (3.0e-3)
#define ABS_ERROR_CHOL (3.0e-2)

/* Upper bound of maximum matrix dimension used by Python */
#define MAXMATRIXDIM 40

#define LOADDATA2()                          \
      const float16_t *inp1=input1.ptr();    \
      const float16_t *inp2=input2.ptr();    \
                                             \
      float16_t *ap=a.ptr();                 \
      float16_t *bp=b.ptr();                 \
                                             \
      float16_t *outp=output.ptr();          \
      int16_t *dimsp = dims.ptr();           \
      int nbMatrixes = dims.nbSamples() >> 1;\
      int rows,columns;                      \
      int i;

#define LOADDATA1()                          \
      const float16_t *inp1=input1.ptr();    \
                                             \
      float16_t *ap=a.ptr();                 \
                                             \
      float16_t *outp=output.ptr();          \
      int16_t *dimsp = dims.ptr();           \
      int nbMatrixes = dims.nbSamples() >> 1;\
      int rows,columns;                      \
      int i;

#define PREPAREDATA2()                                                   \
      in1.numRows=rows;                                                  \
      in1.numCols=columns;                                               \
      memcpy((void*)ap,(const void*)inp1,sizeof(float16_t)*rows*columns);\
      in1.pData = ap;                                                    \
                                                                         \
      in2.numRows=rows;                                                  \
      in2.numCols=columns;                                               \
      memcpy((void*)bp,(const void*)inp2,sizeof(float16_t)*rows*columns);\
      in2.pData = bp;                                                    \
                                                                         \
      out.numRows=rows;                                                  \
      out.numCols=columns;                                               \
      out.pData = outp;

#define PREPAREDATALT()                                                  \
      in1.numRows=rows;                                                  \
      in1.numCols=rows;                                                  \
      memcpy((void*)ap,(const void*)inp1,sizeof(float16_t)*rows*rows);   \
      in1.pData = ap;                                                    \
                                                                         \
      in2.numRows=rows;                                                  \
      in2.numCols=columns;                                               \
      memcpy((void*)bp,(const void*)inp2,sizeof(float16_t)*rows*columns);\
      in2.pData = bp;                                                    \
                                                                         \
      out.numRows=rows;                                                  \
      out.numCols=columns;                                               \
      out.pData = outp;

#define PREPAREDATA1(TRANSPOSED)                                         \
      in1.numRows=rows;                                                  \
      in1.numCols=columns;                                               \
      memcpy((void*)ap,(const void*)inp1,sizeof(float16_t)*rows*columns);\
      in1.pData = ap;                                                    \
                                                                         \
      if (TRANSPOSED)                                                    \
      {                                                                  \
         out.numRows=columns;                                            \
         out.numCols=rows;                                               \
      }                                                                  \
      else                                                               \
      {                                                                  \
      out.numRows=rows;                                                  \
      out.numCols=columns;                                               \
      }                                                                  \
      out.pData = outp;

#define PREPAREDATA1C(TRANSPOSED)                                         \
      in1.numRows=rows;                                                  \
      in1.numCols=columns;                                               \
      memcpy((void*)ap,(const void*)inp1,2*sizeof(float16_t)*rows*columns);\
      in1.pData = ap;                                                    \
                                                                         \
      if (TRANSPOSED)                                                    \
      {                                                                  \
         out.numRows=columns;                                            \
         out.numCols=rows;                                               \
      }                                                                  \
      else                                                               \
      {                                                                  \
      out.numRows=rows;                                                  \
      out.numCols=columns;                                               \
      }                                                                  \
      out.pData = outp;

#define LOADVECDATA2()                          \
      const float16_t *inp1=input1.ptr();    \
      const float16_t *inp2=input2.ptr();    \
                                             \
      float16_t *ap=a.ptr();                 \
      float16_t *bp=b.ptr();                 \
                                             \
      float16_t *outp=output.ptr();          \
      int16_t *dimsp = dims.ptr();           \
      int nbMatrixes = dims.nbSamples() / 2;\
      int rows,internal;                      \
      int i;

#define PREPAREVECDATA2()                                                 \
      in1.numRows=rows;                                                   \
      in1.numCols=internal;                                               \
      memcpy((void*)ap,(const void*)inp1,sizeof(float16_t)*rows*internal);\
      in1.pData = ap;                                                     \
                                                                          \
      memcpy((void*)bp,(const void*)inp2,sizeof(float16_t)*internal);
                            

static void checkInnerTailOverflow(float16_t *b)
{
    ASSERT_TRUE((float32_t)b[0] == 0.0f);
    ASSERT_TRUE((float32_t)b[1] == 0.0f);
    ASSERT_TRUE((float32_t)b[2] == 0.0f);
    ASSERT_TRUE((float32_t)b[3] == 0.0f);
    ASSERT_TRUE((float32_t)b[4] == 0.0f);
    ASSERT_TRUE((float32_t)b[5] == 0.0f);
    ASSERT_TRUE((float32_t)b[6] == 0.0f);
    ASSERT_TRUE((float32_t)b[7] == 0.0f);
}


void UnaryTestsF16::test_householder_f16()
{
   int16_t vecDim;
   const int16_t *dimsp = dims.ptr();          
   const int nbVectors = dims.nbSamples();
   const float16_t *inp1=input1.ptr(); 

   float16_t *outp=output.ptr();   
   float16_t *outBetap=outputBeta.ptr();  


   for(int i=0; i < nbVectors ; i++)
   {
      vecDim = *dimsp++;

      float16_t beta = arm_householder_f16(inp1,DEFAULT_HOUSEHOLDER_THRESHOLD_F16,vecDim,outp);
      *outBetap = beta; 

      outp += vecDim;
      inp1 += vecDim;
      outBetap++;
      checkInnerTailOverflow(outp);
      checkInnerTailOverflow(outBetap);

   }

   ASSERT_EMPTY_TAIL(output);
   ASSERT_EMPTY_TAIL(outputBeta);

   ASSERT_SNR(output,ref,(float32_t)SNR_THRESHOLD);
   ASSERT_SNR(outputBeta,refBeta,(float32_t)SNR_THRESHOLD);

   ASSERT_CLOSE_ERROR(output,ref,ABS_ERROR,REL_ERROR);
   ASSERT_CLOSE_ERROR(outputBeta,refBeta,ABS_ERROR,REL_ERROR);

  
}

void UnaryTestsF16::test_mat_qr_f16()
{
   int16_t rows, columns, rank;
   const int16_t *dimsp = dims.ptr();          
   const int nbMatrixes = dims.nbSamples() / 3;
   const float16_t *inp1=input1.ptr(); 

   float16_t *outTaup=outputTau.ptr();
   float16_t *outRp=outputR.ptr(); 
   float16_t *outQp=outputQ.ptr();  
   float16_t *pTmpA=a.ptr();  
   float16_t *pTmpB=b.ptr();  

   (void) outTaup;
   (void) outRp; 

   for(int i=0; i < nbMatrixes ; i++)
   {
      rows = *dimsp++;
      columns = *dimsp++;
      rank = *dimsp++;
      (void)rank;


      in1.numRows=rows;
      in1.numCols=columns;
      in1.pData = (float16_t*)inp1;

     
      outR.numRows = rows;
      outR.numCols = columns;
      outR.pData = (float16_t*)outRp;

      outQ.numRows = rows;
      outQ.numCols = rows;
      outQ.pData = (float16_t*)outQp;

      arm_status status=arm_mat_qr_f16(&in1,DEFAULT_HOUSEHOLDER_THRESHOLD_F16,&outR,&outQ,outTaup,pTmpA,pTmpB);
      ASSERT_TRUE(status==ARM_MATH_SUCCESS);


      inp1 += rows * columns;
      outRp += rows * columns;
      outQp += rows * rows;
      outTaup += columns;

      checkInnerTailOverflow(outRp);
      checkInnerTailOverflow(outQp);
      checkInnerTailOverflow(outTaup);


   }

   ASSERT_EMPTY_TAIL(outputR);
   ASSERT_EMPTY_TAIL(outputQ);
   ASSERT_EMPTY_TAIL(outputTau);

   //ASSERT_SNR(refQ,outputQ,(float16_t)SNR_QR_THRESHOLD);
   //ASSERT_SNR(refR,outputR,(float16_t)SNR_QR_THRESHOLD);
   //ASSERT_SNR(refTau,outputTau,(float16_t)SNR_QR_THRESHOLD);

   ASSERT_CLOSE_ERROR(refQ,outputQ,ABS_QR_ERROR,REL_QR_ERROR);
   ASSERT_CLOSE_ERROR(refR,outputR,ABS_QR_ERROR,REL_QR_ERROR);
   ASSERT_CLOSE_ERROR(refTau,outputTau,ABS_QR_ERROR,REL_QR_ERROR);
}

void UnaryTestsF16::test_mat_vec_mult_f16()
    {     
      LOADVECDATA2();

      for(i=0;i < nbMatrixes ; i ++)
      {
          rows = *dimsp++;
          internal = *dimsp++;

          PREPAREVECDATA2();

          arm_mat_vec_mult_f16(&this->in1, bp, outp);

          outp += rows ;

      }

      ASSERT_EMPTY_TAIL(output);

      ASSERT_SNR(output,ref,(float32_t)SNR_THRESHOLD);

      ASSERT_CLOSE_ERROR(output,ref,ABS_ERROR,REL_ERROR);

    } 

    void UnaryTestsF16::test_mat_add_f16()
    {     
      LOADDATA2();
      arm_status status;

      for(i=0;i < nbMatrixes ; i ++)
      {
          rows = *dimsp++;
          columns = *dimsp++;

          PREPAREDATA2();

          status=arm_mat_add_f16(&this->in1,&this->in2,&this->out);
          ASSERT_TRUE(status==ARM_MATH_SUCCESS);

          outp += (rows * columns);

      }

      ASSERT_EMPTY_TAIL(output);

      ASSERT_SNR(output,ref,(float32_t)SNR_THRESHOLD);

      ASSERT_CLOSE_ERROR(output,ref,ABS_ERROR,REL_ERROR);

    } 

void UnaryTestsF16::test_mat_sub_f16()
    {     
      LOADDATA2();
      arm_status status;

      for(i=0;i < nbMatrixes ; i ++)
      {
          rows = *dimsp++;
          columns = *dimsp++;

          PREPAREDATA2();

          status=arm_mat_sub_f16(&this->in1,&this->in2,&this->out);
          ASSERT_TRUE(status==ARM_MATH_SUCCESS);

          outp += (rows * columns);

      }

      ASSERT_EMPTY_TAIL(output);

      ASSERT_SNR(output,ref,(float32_t)SNR_THRESHOLD);

      ASSERT_CLOSE_ERROR(output,ref,ABS_ERROR,REL_ERROR);

    } 

void UnaryTestsF16::test_mat_scale_f16()
    {     
      LOADDATA1();
      arm_status status;

      for(i=0;i < nbMatrixes ; i ++)
      {
          rows = *dimsp++;
          columns = *dimsp++;

          PREPAREDATA1(false);

          status=arm_mat_scale_f16(&this->in1,0.5f,&this->out);
          ASSERT_TRUE(status==ARM_MATH_SUCCESS);

          outp += (rows * columns);

      }

      ASSERT_EMPTY_TAIL(output);

      ASSERT_SNR(output,ref,(float32_t)SNR_THRESHOLD);

      ASSERT_CLOSE_ERROR(output,ref,ABS_ERROR,REL_ERROR);

    } 

void UnaryTestsF16::test_mat_trans_f16()
    {     
      LOADDATA1();
      arm_status status;

      for(i=0;i < nbMatrixes ; i ++)
      {
          rows = *dimsp++;
          columns = *dimsp++;

          PREPAREDATA1(true);

          status=arm_mat_trans_f16(&this->in1,&this->out);
          ASSERT_TRUE(status==ARM_MATH_SUCCESS);

          outp += (rows * columns);

      }

      ASSERT_EMPTY_TAIL(output);

      ASSERT_SNR(output,ref,(float32_t)SNR_THRESHOLD);

      ASSERT_CLOSE_ERROR(output,ref,ABS_ERROR,REL_ERROR);

    } 

void UnaryTestsF16::test_mat_cmplx_trans_f16()
    {     
      LOADDATA1();
      arm_status status;

      for(i=0;i < nbMatrixes ; i ++)
      {
          rows = *dimsp++;
          columns = *dimsp++;

          PREPAREDATA1C(true);

          status=arm_mat_cmplx_trans_f16(&this->in1,&this->out);
          ASSERT_TRUE(status==ARM_MATH_SUCCESS);

          outp += 2*(rows * columns);

      }

      ASSERT_EMPTY_TAIL(output);

      ASSERT_SNR(output,ref,(float32_t)SNR_THRESHOLD);

      ASSERT_CLOSE_ERROR(output,ref,ABS_ERROR,REL_ERROR);

    }

static void refInnerTail(float16_t *b)
{
    b[0] = 1.0f;
    b[1] = -2.0f;
    b[2] = 3.0f;
    b[3] = -4.0f;
}

static void checkInnerTail(float16_t *b)
{
    ASSERT_TRUE((float32_t)b[0] == 1.0f);
    ASSERT_TRUE((float32_t)b[1] == -2.0f);
    ASSERT_TRUE((float32_t)b[2] == 3.0f);
    ASSERT_TRUE((float32_t)b[3] == -4.0f);
}

void UnaryTestsF16::test_mat_inverse_f16()
    {     
      const float16_t *inp1=input1.ptr();    
                                             
      float16_t *ap=a.ptr();                 
                                             
      float16_t *outp=output.ptr();          
      int16_t *dimsp = dims.ptr();           
      int nbMatrixes = dims.nbSamples();
      int rows,columns;                      
      int i;
      arm_status status;

      // Non singular matrixes
      // Last matrix is singular
      for(i=0;i < nbMatrixes-1 ; i ++)
      {
          rows = *dimsp++;
          columns = rows;

          PREPAREDATA1(false);

          refInnerTail(outp+(rows * columns));

          status=arm_mat_inverse_f16(&this->in1,&this->out);
          ASSERT_TRUE(status==ARM_MATH_SUCCESS);

          outp += (rows * columns);
          inp1 += (rows * columns);

          checkInnerTail(outp);

      }
      /*** Singular matrix **/
      rows = *dimsp++;
      columns = rows;

      PREPAREDATA1(false);

      refInnerTail(outp+(rows * columns));

      status=arm_mat_inverse_f16(&this->in1,&this->out);
      ASSERT_TRUE(status==ARM_MATH_SINGULAR);

      outp += (rows * columns);
      inp1 += (rows * columns);

      checkInnerTail(outp);
      /**********************/


      ASSERT_SNR(output,ref,(float32_t)SNR_THRESHOLD_INV);

      ASSERT_CLOSE_ERROR(output,ref,ABS_ERROR_INV,REL_ERROR_INV);

    }

    void UnaryTestsF16::test_mat_cholesky_dpo_f16()
    {
      float16_t *ap=a.ptr();                 
      const float16_t *inp1=input1.ptr();    
                                             
                                             
      float16_t *outp=output.ptr();     
      int16_t *dimsp = dims.ptr();           
      int nbMatrixes = dims.nbSamples();

      int rows,columns;                      
      int i;
      arm_status status;

      for(i=0;i < nbMatrixes ; i ++)
      {
          rows = *dimsp++;
          columns = rows;

          PREPAREDATA1(false);

          status=arm_mat_cholesky_f16(&this->in1,&this->out);
          ASSERT_TRUE(status==ARM_MATH_SUCCESS);

          outp += (rows * columns);
          inp1 += (rows * columns);

      }

      ASSERT_EMPTY_TAIL(output);

      ASSERT_SNR(output,ref,(float32_t)SNR_THRESHOLD_CHOL);

      ASSERT_CLOSE_ERROR(ref,output,ABS_ERROR_CHOL,REL_ERROR_CHOL);
    }

    void UnaryTestsF16::test_solve_upper_triangular_f16()
    {
      float16_t *ap=a.ptr();                 
      const float16_t *inp1=input1.ptr();    

      float16_t *bp=b.ptr();                 
      const float16_t *inp2=input2.ptr();    
                                             
                                             
      float16_t *outp=output.ptr();     
      int16_t *dimsp = dims.ptr();           
      int nbMatrixes = dims.nbSamples() >> 1;

      int rows,columns;                      
      int i;
      arm_status status;

      for(i=0;i < nbMatrixes ; i ++)
      {
          rows = *dimsp++;
          columns = *dimsp++;

          PREPAREDATALT();

          status=arm_mat_solve_upper_triangular_f16(&this->in1,&this->in2,&this->out);
          ASSERT_TRUE(status==ARM_MATH_SUCCESS);

          outp += (rows * columns);
          inp1 += (rows * rows);
          inp2 += (rows * columns);


      }

      ASSERT_EMPTY_TAIL(output);

      ASSERT_SNR(output,ref,(float32_t)SNR_THRESHOLD);

      ASSERT_CLOSE_ERROR(ref,output,ABS_ERROR_SOLVE,REL_ERROR_SOLVE);
    }

    void UnaryTestsF16::test_solve_lower_triangular_f16()
    {
      float16_t *ap=a.ptr();                 
      const float16_t *inp1=input1.ptr();    

      float16_t *bp=b.ptr();                 
      const float16_t *inp2=input2.ptr();    
                                             
                                             
      float16_t *outp=output.ptr();     
      int16_t *dimsp = dims.ptr();           
      int nbMatrixes = dims.nbSamples()>>1;

      int rows,columns;                      
      int i;
      arm_status status;

      for(i=0;i < nbMatrixes ; i ++)
      {
          rows = *dimsp++;
          columns = *dimsp++;

          PREPAREDATALT();

          status=arm_mat_solve_lower_triangular_f16(&this->in1,&this->in2,&this->out);
          ASSERT_TRUE(status==ARM_MATH_SUCCESS);

          outp += (rows * columns);
          inp1 += (rows * rows);
          inp2 += (rows * columns);

      }

      ASSERT_EMPTY_TAIL(output);

      ASSERT_SNR(output,ref,(float32_t)SNR_THRESHOLD);

      ASSERT_CLOSE_ERROR(ref,output,ABS_ERROR_SOLVE,REL_ERROR_SOLVE);
    }

    void UnaryTestsF16::setUp(Testing::testID_t id,std::vector<Testing::param_t>& params,Client::PatternMgr *mgr)
    {


      (void)params;
      switch(id)
      {
         case TEST_MAT_ADD_F16_1:
            input1.reload(UnaryTestsF16::INPUTS1_F16_ID,mgr);
            input2.reload(UnaryTestsF16::INPUTS2_F16_ID,mgr);
            dims.reload(UnaryTestsF16::DIMSUNARY1_S16_ID,mgr);

            ref.reload(UnaryTestsF16::REFADD1_F16_ID,mgr);

            output.create(ref.nbSamples(),UnaryTestsF16::OUT_F16_ID,mgr);
            a.create(MAXMATRIXDIM*MAXMATRIXDIM,UnaryTestsF16::TMPA_F16_ID,mgr);
            b.create(MAXMATRIXDIM*MAXMATRIXDIM,UnaryTestsF16::TMPB_F16_ID,mgr);
         break;

         case TEST_MAT_SUB_F16_2:
            input1.reload(UnaryTestsF16::INPUTS1_F16_ID,mgr);
            input2.reload(UnaryTestsF16::INPUTS2_F16_ID,mgr);
            dims.reload(UnaryTestsF16::DIMSUNARY1_S16_ID,mgr);

            ref.reload(UnaryTestsF16::REFSUB1_F16_ID,mgr);

            output.create(ref.nbSamples(),UnaryTestsF16::OUT_F16_ID,mgr);
            a.create(MAXMATRIXDIM*MAXMATRIXDIM,UnaryTestsF16::TMPA_F16_ID,mgr);
            b.create(MAXMATRIXDIM*MAXMATRIXDIM,UnaryTestsF16::TMPB_F16_ID,mgr);
         break;

         case TEST_MAT_SCALE_F16_3:
            input1.reload(UnaryTestsF16::INPUTS1_F16_ID,mgr);
            dims.reload(UnaryTestsF16::DIMSUNARY1_S16_ID,mgr);

            ref.reload(UnaryTestsF16::REFSCALE1_F16_ID,mgr);

            output.create(ref.nbSamples(),UnaryTestsF16::OUT_F16_ID,mgr);
            a.create(MAXMATRIXDIM*MAXMATRIXDIM,UnaryTestsF16::TMPA_F16_ID,mgr);
         break;

         case TEST_MAT_TRANS_F16_4:
            input1.reload(UnaryTestsF16::INPUTS1_F16_ID,mgr);
            dims.reload(UnaryTestsF16::DIMSUNARY1_S16_ID,mgr);

            ref.reload(UnaryTestsF16::REFTRANS1_F16_ID,mgr);

            output.create(ref.nbSamples(),UnaryTestsF16::OUT_F16_ID,mgr);
            a.create(MAXMATRIXDIM*MAXMATRIXDIM,UnaryTestsF16::TMPA_F16_ID,mgr);
         break;

         case TEST_MAT_INVERSE_F16_5:
            input1.reload(UnaryTestsF16::INPUTSINV_F16_ID,mgr);
            dims.reload(UnaryTestsF16::DIMSINVERT1_S16_ID,mgr);

            ref.reload(UnaryTestsF16::REFINV1_F16_ID,mgr);

            output.create(ref.nbSamples(),UnaryTestsF16::OUT_F16_ID,mgr);
            a.create(MAXMATRIXDIM*MAXMATRIXDIM,UnaryTestsF16::TMPA_F16_ID,mgr);
         break;

         case TEST_MAT_VEC_MULT_F16_6:
            input1.reload(UnaryTestsF16::INPUTS1_F16_ID,mgr);
            input2.reload(UnaryTestsF16::INPUTVEC1_F16_ID,mgr);
            dims.reload(UnaryTestsF16::DIMSUNARY1_S16_ID,mgr);

            ref.reload(UnaryTestsF16::REFVECMUL1_F16_ID,mgr);

            output.create(ref.nbSamples(),UnaryTestsF16::OUT_F16_ID,mgr);
            a.create(MAXMATRIXDIM*MAXMATRIXDIM,UnaryTestsF16::TMPA_F16_ID,mgr);
            b.create(MAXMATRIXDIM,UnaryTestsF16::TMPB_F16_ID,mgr);
         break;

          case TEST_MAT_CMPLX_TRANS_F16_7:
            input1.reload(UnaryTestsF16::INPUTSC1_F16_ID,mgr);
            dims.reload(UnaryTestsF16::DIMSUNARY1_S16_ID,mgr);

            ref.reload(UnaryTestsF16::REFTRANSC1_F16_ID,mgr);

            output.create(ref.nbSamples(),UnaryTestsF16::OUT_F16_ID,mgr);
            a.create(MAXMATRIXDIM*MAXMATRIXDIM,UnaryTestsF16::TMPA_F16_ID,mgr);
         break;

         case TEST_MAT_CHOLESKY_DPO_F16_8:
            input1.reload(UnaryTestsF16::INPUTSCHOLESKY1_DPO_F16_ID,mgr);
            dims.reload(UnaryTestsF16::DIMSCHOLESKY1_DPO_S16_ID,mgr);

            ref.reload(UnaryTestsF16::REFCHOLESKY1_DPO_F16_ID,mgr);

            output.create(ref.nbSamples(),UnaryTestsF16::OUT_F16_ID,mgr);
            a.create(MAXMATRIXDIM*MAXMATRIXDIM,UnaryTestsF16::TMPA_F16_ID,mgr);
         break;

         case TEST_SOLVE_UPPER_TRIANGULAR_F16_9:
            input1.reload(UnaryTestsF16::INPUT_MAT_UTSOLVE_F16_ID,mgr);
            input2.reload(UnaryTestsF16::INPUT_VEC_LTSOLVE_F16_ID,mgr);
            dims.reload(UnaryTestsF16::DIM_LTSOLVE_F16_ID,mgr);

            ref.reload(UnaryTestsF16::REF_UT_SOLVE_F16_ID,mgr);

            output.create(ref.nbSamples(),UnaryTestsF16::OUT_F16_ID,mgr);
            a.create(MAXMATRIXDIM*MAXMATRIXDIM,UnaryTestsF16::TMPA_F16_ID,mgr);
            b.create(MAXMATRIXDIM*MAXMATRIXDIM,UnaryTestsF16::TMPB_F16_ID,mgr);
         break;

         case TEST_SOLVE_LOWER_TRIANGULAR_F16_10:
            input1.reload(UnaryTestsF16::INPUT_MAT_LTSOLVE_F16_ID,mgr);
            input2.reload(UnaryTestsF16::INPUT_VEC_LTSOLVE_F16_ID,mgr);
            dims.reload(UnaryTestsF16::DIM_LTSOLVE_F16_ID,mgr);

            ref.reload(UnaryTestsF16::REF_LT_SOLVE_F16_ID,mgr);

            output.create(ref.nbSamples(),UnaryTestsF16::OUT_F16_ID,mgr);
            a.create(MAXMATRIXDIM*MAXMATRIXDIM,UnaryTestsF16::TMPA_F16_ID,mgr);
            b.create(MAXMATRIXDIM*MAXMATRIXDIM,UnaryTestsF16::TMPB_F16_ID,mgr);
         break;

         case TEST_HOUSEHOLDER_F16_11:
            input1.reload(UnaryTestsF16::INPUTS_HOUSEHOLDER_F16_ID,mgr);
            dims.reload(UnaryTestsF16::DIMS_HOUSEHOLDER_S16_ID,mgr);
            ref.reload(UnaryTestsF16::REF_HOUSEHOLDER_V_F16_ID,mgr);
            refBeta.reload(UnaryTestsF16::REF_HOUSEHOLDER_BETA_F16_ID,mgr);


            output.create(ref.nbSamples(),UnaryTestsF16::TMPA_F16_ID,mgr);
            outputBeta.create(refBeta.nbSamples(),UnaryTestsF16::TMPB_F16_ID,mgr);
         break;


         case TEST_MAT_QR_F16_12:
            input1.reload(UnaryTestsF16::INPUTS_QR_F16_ID,mgr);
            dims.reload(UnaryTestsF16::DIMS_QR_S16_ID,mgr);
            refTau.reload(UnaryTestsF16::REF_QR_TAU_F16_ID,mgr);
            refR.reload(UnaryTestsF16::REF_QR_R_F16_ID,mgr);
            refQ.reload(UnaryTestsF16::REF_QR_Q_F16_ID,mgr);


            outputTau.create(refTau.nbSamples(),UnaryTestsF16::TMPA_F16_ID,mgr);
            outputR.create(refR.nbSamples(),UnaryTestsF16::TMPB_F16_ID,mgr);
            outputQ.create(refQ.nbSamples(),UnaryTestsF16::TMPC_F16_ID,mgr);

            a.create(47,UnaryTestsF16::TMPC_F16_ID,mgr);
            b.create(47,UnaryTestsF16::TMPD_F16_ID,mgr);
         break;
      }
       

    
    }

    void UnaryTestsF16::tearDown(Testing::testID_t id,Client::PatternMgr *mgr)
    {
       (void)id;
       //output.dump(mgr);
       (void)mgr;
    }
