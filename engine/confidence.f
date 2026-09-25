C     HyperTension Statistical Confidence Authority or something
      PROGRAM CONFID
      INTEGER N, CFND, BFPOS, AGREE, CIDX, AIDX
      DOUBLE PRECISION U, CONFD
      DOUBLE PRECISION BASE, RAGREE, RBFYES, RBFNO, RNDIV
      PARAMETER (BASE   = 0.25D0)
      PARAMETER (RAGREE = 0.05D0)
      PARAMETER (RBFYES = 0.35D0)
      PARAMETER (RBFNO  = 2.00D0)
      PARAMETER (RNDIV  = 0.10D0)

C     Read the input.
      READ(*,*) N, CFND, BFPOS, AGREE, CIDX, AIDX

C     Initialize U.
      U = BASE

      IF (AGREE .EQ. 1) THEN
          U = U * RAGREE
      END IF

C     Apply BF evidence factor.
      IF (CFND .EQ. BFPOS) THEN
          U = U * RBFYES
      ELSE
          U = U * RBFNO
      END IF

      U = U / (1.0D0 + RNDIV * DBLE(N))

      CONFD = 100.0D0 * (1.0D0 - U)

C     Clamp the result to [0, 100]. Values outside this range indicate
C     a model parameterization error and must not be emitted as-is.
      IF (CONFD .GT. 100.0D0) CONFD = 100.0D0
      IF (CONFD .LT.   0.0D0) CONFD = 0.0D0

C     Write the confidence.
      WRITE(*,'(F8.4)') CONFD

      STOP
      END
