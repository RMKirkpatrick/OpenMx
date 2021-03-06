library(OpenMx)
covariance <- matrix(c(1.0, 0.5, 0.5, 1.0), nrow=2, dimnames=list(c("a", "b"),
                                                                  c("a", "b")))
means <- c(-1,.5)
names(means) <- c('a','b')

model <- mxModel("CI Example",
                 mxMatrix(name="expectedCov", "Symm", 2, 2, free=T, values = c(1, 0, 1),
                          labels = c("var1", "cov12", "var2")),
                 mxMatrix(name="expectedMean", "Full", 1, 2, free=T, labels=c('m1','m2')),
                 mxExpectationNormal("expectedCov", "expectedMean", dimnames=c("a", "b")),
                 mxFitFunctionML(),
                 mxData(covariance, "cov", means, numObs=10000)
)

model <- mxOption(model,"Checkpoint Units",'iterations')
model <- mxOption(model,"Checkpoint Count",1)

fit1 <- mxRun(model, silent=TRUE)

cimodel <- mxModel(model,
                   mxCI("var1", type="lower"),
                   mxCI("cov12", type="upper"),
                   mxCI("m1", type="both"))
fit2 <- mxRun(cimodel,
              intervals = TRUE, silent=TRUE, checkpoint=FALSE)

# For multivariate normal means, SEs match likelihood-based CIs
omxCheckCloseEnough(fit2$output$estimate['m1'] + fit2$output$standardErrors['m1',] * qnorm(.025),
                    fit2$output$confidenceIntervals['m1', 'lbound'], .0001)
omxCheckCloseEnough(fit2$output$estimate['m1'] - fit2$output$standardErrors['m1',] * qnorm(.025),
                    fit2$output$confidenceIntervals['m1', 'ubound'], .0001)

# cat(deparse(round(model$output$confidenceIntervals, 3)))
omxCheckCloseEnough(fit2$output$confidenceIntervals['var1','lbound'], c(0.973), .01)
omxCheckCloseEnough(fit2$output$confidenceIntervals['cov12','ubound'], c(0.522), .01)

omxCheckCloseEnough(fit1$output$fit, fit2$output$fit, 1e-6)
omxCheckCloseEnough(fit1$output$standardErrors, fit2$output$standardErrors, 1e-6)
